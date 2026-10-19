use crate::{
    error::ProcessError, event::ServiceContext, http::body_type::ResponseBody,
    resource_cache::ResourceRequest,
};
use http_body_util::{BodyExt, Full, Limited};
use hyper::{
    body::{Bytes, Incoming},
    Method, Request, Response, StatusCode,
};
use qcm_core::{
    provider::{PlaybackKind, PlaybackRequest, PreparedPlayback, Provider},
    resource::{PreparedResource, ResourceLifetime},
};
use reqwest::Url;
use sea_orm::{EntityTrait, QuerySelect};
use serde_json::json;
use std::{
    collections::HashMap,
    sync::Arc,
    time::{Duration, Instant},
};
use tokio::sync::{Mutex, OwnedSemaphorePermit, Semaphore};

const MAX_SESSIONS: usize = 16;
const IDLE: Duration = Duration::from_secs(120);

#[cfg(test)]
#[path = "playback_tests.rs"]
mod tests;

pub struct PlaybackSessions {
    sessions: Mutex<HashMap<String, Arc<Session>>>,
    closing: std::sync::Mutex<Vec<(Option<i64>, tokio::task::JoinHandle<()>)>>,
    stopping: std::sync::atomic::AtomicBool,
    slots: Arc<Semaphore>,
}
impl Default for PlaybackSessions {
    fn default() -> Self {
        Self {
            sessions: Mutex::new(HashMap::new()),
            closing: std::sync::Mutex::new(Vec::new()),
            stopping: std::sync::atomic::AtomicBool::new(false),
            slots: Arc::new(Semaphore::new(MAX_SESSIONS)),
        }
    }
}
struct Session {
    _permit: OwnedSemaphorePermit,
    token: String,
    provider: Arc<dyn Provider>,
    epoch: u64,
    source: PreparedPlayback,
    valid: Arc<ResourceLifetime>,
    touched: Mutex<Instant>,
    origins: Mutex<Vec<Url>>,
    prepared: Mutex<HashMap<String, Arc<PreparedResource>>>,
    redirects: Mutex<HashMap<String, (Instant, Response<ResponseBody>)>>,
}
impl Session {
    fn valid(&self) -> bool {
        self.valid.is_valid() && self.epoch == self.provider.lifecycle_epoch()
    }
    async fn prepare(&self, uri: &str) -> Result<Arc<PreparedResource>, ProcessError> {
        if !self.valid() {
            return Err(ProcessError::NotAuth);
        }
        if let Some(resource) = self.prepared.lock().await.remove(uri) {
            return Ok(resource);
        }
        let resource = self
            .provider
            .prepare_playback_resource(&self.source.state, uri)
            .await?;
        let valid = self.valid.clone();
        // Keep both the account guard supplied by the provider and the playback lifetime.
        Ok(Arc::new(resource.with_lifetime(valid)))
    }
    async fn route(&self, uri: &Url) -> Result<String, ProcessError> {
        if !matches!(uri.scheme(), "http" | "https")
            || !uri.username().is_empty()
            || uri.password().is_some()
            || uri.fragment().is_some()
        {
            return Err(ProcessError::NotFound);
        }
        let mut origins = self.origins.lock().await;
        let index = if let Some(i) = origins.iter().position(|o| o.origin() == uri.origin()) {
            i
        } else {
            if origins.len() >= 8 {
                return Err(ProcessError::Resource(qcm_core::anyhow!(
                    "playback origin limit"
                )));
            }
            origins.push(uri.clone());
            origins.len() - 1
        };
        Ok(format!(
            "/playback/{}/r/{index}{}{}",
            self.token,
            uri.path(),
            uri.query().map(|q| format!("?{q}")).unwrap_or_default()
        ))
    }
    async fn cleanup(&self) {
        self.prepared.lock().await.clear();
        self.redirects.lock().await.clear();
        for _ in 0..3 {
            match tokio::time::timeout(
                Duration::from_secs(5),
                self.provider.close_playback(&self.source.state),
            )
            .await
            {
                Ok(Ok(())) => return,
                _ => tokio::task::yield_now().await,
            }
        }
        log::warn!("playback session cleanup did not complete");
    }
}
impl PlaybackSessions {
    async fn get(&self, token: &str) -> Result<Arc<Session>, ProcessError> {
        let session = self
            .sessions
            .lock()
            .await
            .get(token)
            .cloned()
            .ok_or(ProcessError::NotFound)?;
        if !session.valid() {
            return Err(ProcessError::NotAuth);
        }
        *session.touched.lock().await = Instant::now();
        Ok(session)
    }
    // Live-session removal and cleanup registration share the sessions lock.
    fn schedule_close(&self, session: Arc<Session>) {
        if !session.valid.cancel() {
            return;
        }
        let mut tasks = self.closing.lock().unwrap();
        tasks.retain(|(_, task)| !task.is_finished());
        tasks.push((
            session.provider.id(),
            tokio::spawn(async move {
                session.cleanup().await;
            }),
        ));
    }
    async fn finish_closing(&self, provider: Option<i64>) {
        let tasks = {
            let mut closing = self.closing.lock().unwrap();
            let mut selected = Vec::new();
            let mut retained = Vec::new();
            for (owner, task) in closing.drain(..) {
                if provider.is_none() || owner == provider {
                    selected.push(task);
                } else {
                    retained.push((owner, task));
                }
            }
            *closing = retained;
            selected
        };
        for task in tasks {
            let _ = task.await;
        }
    }
    pub async fn collect(&self) {
        let sessions: Vec<_> = self.sessions.lock().await.values().cloned().collect();
        let mut heartbeats = tokio::task::JoinSet::new();
        for session in sessions {
            if !session.valid() || session.touched.lock().await.elapsed() > IDLE {
                let mut sessions = self.sessions.lock().await;
                if let Some(removed) = sessions.remove(&session.token) {
                    self.schedule_close(removed);
                }
            } else {
                session
                    .redirects
                    .lock()
                    .await
                    .retain(|_, (at, _)| at.elapsed() < Duration::from_secs(15));
                heartbeats.spawn(async move {
                    let _ = tokio::time::timeout(
                        Duration::from_secs(5),
                        session.provider.keep_playback(&session.source.state),
                    )
                    .await;
                });
            }
        }
        while heartbeats.join_next().await.is_some() {}
    }
    pub async fn invalidate(&self, provider: i64) {
        let mut sessions = self.sessions.lock().await;
        let tokens: Vec<_> = sessions
            .iter()
            .filter(|(_, s)| s.provider.id() == Some(provider))
            .map(|(k, _)| k.clone())
            .collect();
        let removed: Vec<_> = tokens
            .into_iter()
            .filter_map(|k| sessions.remove(&k))
            .collect();
        for session in removed {
            self.schedule_close(session);
        }
        drop(sessions);
        self.finish_closing(Some(provider)).await;
    }
    pub async fn shutdown(&self) {
        self.stopping
            .store(true, std::sync::atomic::Ordering::Release);
        self.slots.close();
        {
            let mut sessions = self.sessions.lock().await;
            for (_, session) in sessions.drain() {
                self.schedule_close(session);
            }
        }
        self.finish_closing(None).await;
    }
}
fn json_response(value: serde_json::Value) -> Result<Response<ResponseBody>, ProcessError> {
    let mut response = Response::new(ResponseBody::Boxed(
        Full::new(Bytes::from(
            serde_json::to_vec(&value).map_err(qcm_core::Error::from)?,
        ))
        .map_err(|e| e.into())
        .boxed(),
    ));
    response
        .headers_mut()
        .insert("content-type", "application/json".parse().unwrap());
    response
        .headers_mut()
        .insert("cache-control", "no-store".parse().unwrap());
    Ok(response)
}
async fn body_json(req: Request<Incoming>) -> Result<serde_json::Value, ProcessError> {
    let bytes = Limited::new(req.into_body(), 64 * 1024)
        .collect()
        .await
        .map_err(|_| ProcessError::Resource(qcm_core::anyhow!("playback request limit")))?
        .to_bytes();
    if bytes.is_empty() {
        Ok(json!({}))
    } else {
        serde_json::from_slice(&bytes)
            .map_err(|_| ProcessError::Resource(qcm_core::anyhow!("invalid playback request")))
    }
}
pub async fn handle(
    ctx: &Arc<ServiceContext>,
    req: Request<Incoming>,
) -> Result<Response<ResponseBody>, ProcessError> {
    let path = req.uri().path().to_string();
    let parts: Vec<_> = path.split('/').skip(1).collect();
    if let ["playback", "source", "Song", id] = parts.as_slice() {
        if req.method() != Method::POST {
            return Err(ProcessError::NotFound);
        }
        let id: i64 = id.parse().map_err(|_| ProcessError::NotFound)?;
        let params = body_json(req).await?;
        let max_bitrate = params
            .get("max_bitrate")
            .and_then(|v| v.as_u64())
            .map(u32::try_from)
            .transpose()
            .map_err(|_| ProcessError::NotFound)?;
        use qcm_core::model as m;
        let (item_id, provider_id): (String, i64) = m::song::Entity::find_by_id(id)
            .inner_join(m::item::Entity)
            .select_only()
            .column(m::item::Column::NativeId)
            .column(m::item::Column::ProviderId)
            .into_tuple()
            .one(&ctx.provider_context.db)
            .await?
            .ok_or(ProcessError::NotFound)?;
        let provider = qcm_core::global::provider(provider_id).ok_or(ProcessError::NotFound)?;
        let permit = ctx
            .playback
            .slots
            .clone()
            .try_acquire_owned()
            .map_err(|_| ProcessError::Resource(qcm_core::anyhow!("playback session limit")))?;
        let epoch = provider.lifecycle_epoch();
        let source = match provider
            .open_playback(PlaybackRequest {
                item_id,
                max_bitrate,
            })
            .await
        {
            Ok(source) if source.kind == PlaybackKind::ByteResource => {
                return json_response(
                    json!({"url":format!("/audio/Song/{id}"),"session":null,"time_seekable":false}),
                )
            }
            Ok(source) => source,
            Err(error) => return Err(error.into()),
        };
        let token = uuid::Uuid::new_v4().simple().to_string();
        let session = Arc::new(Session {
            _permit: permit,
            token: token.clone(),
            provider,
            epoch,
            source,
            valid: Arc::new(ResourceLifetime::default()),
            touched: Mutex::new(Instant::now()),
            origins: Mutex::new(Vec::new()),
            prepared: Mutex::new(HashMap::new()),
            redirects: Mutex::new(HashMap::new()),
        });
        let registered = async {
            if !session.valid() { return Err(ProcessError::NotAuth); }
            let uri = Url::parse(&session.source.uri).map_err(|_|ProcessError::NotFound)?;
            let prepared = session.prepare(uri.as_str()).await?;
            let route = session.route(&uri).await?;
            session.prepared.lock().await.insert(uri.to_string(),prepared);
            let mut sessions = ctx.playback.sessions.lock().await;
            if ctx.playback.stopping.load(std::sync::atomic::Ordering::Acquire) {return Err(ProcessError::NotFound);}
            if sessions.len()>=MAX_SESSIONS { return Err(ProcessError::Resource(qcm_core::anyhow!("playback session limit"))); }
            sessions.insert(token.clone(),session.clone());
            json_response(json!({"url":route,"session":token,"time_seekable":session.source.kind==PlaybackKind::HlsVod,"duration_seconds":session.source.duration_seconds}))
        }.await;
        if registered.is_err() {
            ctx.playback.schedule_close(session);
        }
        return registered;
    }
    let token = parts.get(1).ok_or(ProcessError::NotFound)?;
    if parts.get(2) == Some(&"close") && req.method() == Method::POST {
        let mut sessions = ctx.playback.sessions.lock().await;
        if let Some(session) = sessions.remove(*token) {
            ctx.playback.schedule_close(session);
        }
        return json_response(json!({}));
    }
    let session = ctx.playback.get(token).await?;
    if parts.get(2) == Some(&"touch") && req.method() == Method::POST {
        return json_response(json!({}));
    }
    if parts.get(2) == Some(&"resolve") && req.method() == Method::POST {
        let value = body_json(req).await?;
        let uri = Url::parse(
            value
                .get("uri")
                .and_then(|v| v.as_str())
                .ok_or(ProcessError::NotFound)?,
        )
        .map_err(|_| ProcessError::NotFound)?;
        let prepared = session.prepare(uri.as_str()).await?;
        let route = session.route(&uri).await?;
        let mut pending = session.prepared.lock().await;
        if pending.len() >= 16 {
            pending.clear();
        }
        pending.insert(uri.to_string(), prepared);
        return json_response(json!({"url":route}));
    }
    if parts.get(2) != Some(&"r")
        || parts.len() < 5
        || !matches!(*req.method(), Method::GET | Method::HEAD)
    {
        return Err(ProcessError::NotFound);
    }
    let index: usize = parts[3].parse().map_err(|_| ProcessError::NotFound)?;
    let mut uri = session
        .origins
        .lock()
        .await
        .get(index)
        .cloned()
        .ok_or(ProcessError::NotFound)?;
    let prefix = format!("/playback/{token}/r/{index}");
    uri.set_path(path.strip_prefix(&prefix).ok_or(ProcessError::NotFound)?);
    uri.set_query(req.uri().query());
    let redirect_key = format!("{}:{:?}", req.uri(), req.headers().get("range"));
    if let Some((_, mut response)) = session.redirects.lock().await.remove(&redirect_key) {
        if req.method() == Method::HEAD {
            *response.body_mut() = ResponseBody::Empty;
        }
        return Ok(response);
    }
    let prepared = session.prepare(uri.as_str()).await?;
    let scope = serde_json::to_string(&(
        session.token.as_str(),
        &session.source.representation,
        session.epoch,
    ))
    .map_err(qcm_core::Error::from)?;
    let request = ResourceRequest {
        provider: session.provider.id().ok_or(ProcessError::NotFound)?,
        kind: 1,
        scope,
        prepared: prepared.clone(),
        headers: req.headers().clone(),
    };
    let (mut response, effective) =
        if prepared.cache_mode() == qcm_core::resource::CacheMode::ByteRange {
            (ctx.resource_cache.response(request).await?, uri.clone())
        } else {
            ctx.resource_cache.object_response(request).await?
        };
    if !session.valid() {
        return Err(ProcessError::NotAuth);
    }
    if effective != uri && response.status().is_success() {
        // Authorization and effective URI belong to the resource owner, before redirect exposure.
        let _ = session.prepare(effective.as_str()).await?;
        let route = session.route(&effective).await?;
        let key = format!("{route}:{:?}", req.headers().get("range"));
        let mut pending = session.redirects.lock().await;
        pending.retain(|_, (at, _)| at.elapsed() < Duration::from_secs(15));
        if pending.len() >= 4 {
            return Err(ProcessError::Resource(qcm_core::anyhow!(
                "playback redirect limit"
            )));
        }
        pending.insert(key, (Instant::now(), response));
        return Ok(Response::builder()
            .status(StatusCode::TEMPORARY_REDIRECT)
            .header("location", route)
            .header("cache-control", "no-store")
            .body(ResponseBody::Empty)
            .unwrap());
    }
    if req.method() == Method::HEAD {
        *response.body_mut() = ResponseBody::Empty;
    }
    Ok(response)
}
