use qcm_core::{
    error::ProviderError,
    http::{CookieStoreRwLock, CookieStoreTrait, HeaderValue, HttpClient},
    model::provider,
};
use sea_orm::sea_query::Expr;
use sea_orm::*;
use std::sync::{
    atomic::{AtomicBool, Ordering},
    Arc, RwLock,
};
use tracing::instrument::WithSubscriber;

pub struct Network {
    client: HttpClient,
    stream: HttpClient,
    policy: qcm_core::network::ClientPolicy,
    pub jar: Arc<CookieStoreRwLock>,
    guard: Arc<SessionCookies>,
}
struct SessionCookies {
    jar: Arc<CookieStoreRwLock>,
    active: AtomicBool,
}
impl CookieStoreTrait for SessionCookies {
    fn set_cookies(&self, headers: &mut dyn Iterator<Item = &HeaderValue>, url: &reqwest::Url) {
        if self.active.load(Ordering::Acquire) {
            self.jar.set_cookies(headers, url);
        }
    }
    fn cookies(&self, url: &reqwest::Url) -> Option<HeaderValue> {
        self.active
            .load(Ordering::Acquire)
            .then(|| self.jar.cookies(url))
            .flatten()
    }
}
impl Network {
    pub fn new() -> qcm_core::Result<Self> {
        let jar = Arc::new(CookieStoreRwLock::default());
        let guard = Arc::new(SessionCookies {
            jar: jar.clone(),
            active: AtomicBool::new(true),
        });
        let policy = qcm_core::network::ClientPolicy::default();
        let client = policy.builder()
            .cookie_provider(guard.clone())
            .build()?;
        let stream = policy.builder()
            .cookie_provider(guard.clone())
            .no_gzip()
            .no_brotli()
            .no_deflate()
            .no_zstd()
            .build()?;
        Ok(Self {
            client,
            stream,
            policy,
            jar,
            guard,
        })
    }
    pub fn invalidate(&self) {
        self.guard.active.store(false, Ordering::Release);
    }
    pub fn clients(&mut self, scope: Option<i64>) -> qcm_core::Result<(HttpClient, HttpClient)> {
        let policy = qcm_core::network::policy(scope);
        if self.policy != policy {
            let client = policy.builder().cookie_provider(self.guard.clone()).build()?;
            let stream = policy.builder().cookie_provider(self.guard.clone())
                .no_gzip().no_brotli().no_deflate().no_zstd().build()?;
            self.client = client;
            self.stream = stream;
            self.policy = policy;
        }
        Ok((self.client.clone(), self.stream.clone()))
    }
    pub fn lua_client(&mut self, scope: Option<i64>) -> qcm_core::Result<mlua_extra::http::LuaHttpClient> {
        let (client, stream) = self.clients(scope)?;
        Ok(crate::http::new_client(client, self.guard.clone()).with_stream_client(stream))
    }

}

pub struct Session {
    binding: RwLock<(String, Option<DatabaseConnection>)>,
    pub writes: tokio::sync::Mutex<()>,
    active: AtomicBool,
}
impl Session {
    pub fn new() -> Self {
        Self {
            binding: RwLock::new((uuid::Uuid::new_v4().to_string(), None)),
            writes: tokio::sync::Mutex::new(()),
            active: AtomicBool::new(true),
        }
    }
    pub fn key(&self) -> String {
        self.binding.read().unwrap().0.clone()
    }
    pub fn bind(&self, db: DatabaseConnection, key: String) {
        *self.binding.write().unwrap() = (key, Some(db));
    }
    pub fn invalidate(&self) {
        self.active.store(false, Ordering::Release);
    }
    pub fn check(&self) -> Result<(), ProviderError> {
        if self.active.load(Ordering::Acquire) {
            Ok(())
        } else {
            Err(ProviderError::NotAuth)
        }
    }
    pub async fn save(
        &self,
        id: Option<i64>,
        custom: String,
        cookie: String,
    ) -> Result<&'static str, ProviderError> {
        self.check()?;
        let (key, db) = self.binding.read().unwrap().clone();
        let (Some(id), Some(db)) = (id, db) else {
            return Ok("staged");
        };
        // The key prevents a late response from overwriting a replacement provider.
        let result = provider::Entity::update_many()
            .col_expr(provider::Column::Custom, Expr::value(custom))
            .col_expr(provider::Column::Cookie, Expr::value(cookie))
            .filter(provider::Column::ProviderId.eq(id))
            .filter(provider::Column::SessionKey.eq(key))
            .exec(&db)
            .with_subscriber(tracing::subscriber::NoSubscriber::default())
            .await?;
        if result.rows_affected != 1 {
            return Err(ProviderError::NotAuth);
        }
        self.check()?;
        Ok("saved")
    }
}

struct Flight {
    result: tokio::sync::watch::Sender<Option<mlua::Result<mlua::Value>>>,
}
struct FlightOwner(Arc<Flight>);
impl Drop for FlightOwner {
    fn drop(&mut self) {
        if self.0.result.borrow().is_none() {
            self.0
                .result
                .send_replace(Some(Err(mlua::Error::runtime("session task cancelled"))));
        }
    }
}
#[derive(Default)]
pub struct Tasks(std::sync::Mutex<std::collections::HashMap<String, std::sync::Weak<Flight>>>);
impl Tasks {
    pub fn cancel(&self) {
        for flight in self
            .0
            .lock()
            .unwrap()
            .drain()
            .filter_map(|(_, f)| f.upgrade())
        {
            flight
                .result
                .send_replace(Some(Err(mlua::Error::runtime("session task cancelled"))));
        }
    }
    pub async fn run(&self, key: String, callback: mlua::Function) -> mlua::Result<mlua::Value> {
        let (flight, owner) = {
            let mut tasks = self.0.lock().unwrap();
            match tasks.get(&key).and_then(std::sync::Weak::upgrade) {
                Some(flight) if flight.result.borrow().is_none() => (flight, false),
                _ => {
                    let (result, _) = tokio::sync::watch::channel(None);
                    let flight = Arc::new(Flight { result });
                    tasks.insert(key, Arc::downgrade(&flight));
                    (flight, true)
                }
            }
        };
        if owner {
            let guard = FlightOwner(flight.clone());
            let result = callback.call_async::<mlua::Value>(()).await;
            let result = flight.result.borrow().clone().unwrap_or(result);
            flight.result.send_replace(Some(result.clone()));
            drop(guard);
            result
        } else {
            let mut rx = flight.result.subscribe();
            loop {
                if let Some(result) = rx.borrow().clone() {
                    return result;
                }
                rx.changed()
                    .await
                    .map_err(|_| mlua::Error::runtime("session task cancelled"))?;
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use mlua::Lua;
    #[tokio::test]
    async fn tasks_share_failures_and_owner_cancellation() {
        let lua = Lua::new();
        let calls = Arc::new(std::sync::atomic::AtomicUsize::new(0));
        let callback = lua
            .create_async_function({
                let calls = calls.clone();
                move |_, (): ()| {
                    let calls = calls.clone();
                    async move {
                        calls.fetch_add(1, Ordering::Relaxed);
                        tokio::time::sleep(std::time::Duration::from_millis(30)).await;
                        Err::<(), _>(mlua::Error::runtime("synthetic failure"))
                    }
                }
            })
            .unwrap();
        let tasks = Tasks::default();
        let (a, b) = tokio::join!(
            tasks.run("same".into(), callback.clone()),
            tasks.run("same".into(), callback.clone())
        );
        assert!(a.is_err() && b.is_err());
        assert_eq!(calls.load(Ordering::Relaxed), 1);
        let owner = tasks.run("cancel".into(), callback.clone());
        let waiter = tasks.run("cancel".into(), callback.clone());
        let mut owner = Box::pin(owner);
        let mut waiter = Box::pin(waiter);
        tokio::select! {_=&mut owner=>panic!("unexpected completion"),_=tokio::time::sleep(std::time::Duration::from_millis(2))=>{}}
        tokio::select! {_=&mut waiter=>panic!("unexpected completion"),_=tokio::time::sleep(std::time::Duration::from_millis(2))=>{}}
        drop(owner);
        assert!(
            tokio::time::timeout(std::time::Duration::from_millis(100), waiter)
                .await
                .unwrap()
                .is_err()
        );
        assert!(tasks.run("cancel".into(), callback).await.is_err());
        assert_eq!(calls.load(Ordering::Relaxed), 3);
    }
}
