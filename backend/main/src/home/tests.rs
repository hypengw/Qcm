use super::*;
use qcm_core::{
    provider::{AuthInfo, AuthResult, HasCommonData, ProviderCommonData, ProviderSession},
    subtitle::Subtitle,
};
use sea_orm::{ActiveModelTrait, Set};
use std::sync::{
    atomic::{AtomicBool, AtomicU64, Ordering},
    Mutex,
};
use tokio::sync::Semaphore;

struct HomeProvider {
    common: ProviderCommonData,
    epoch: AtomicU64,
    entries: Mutex<Option<Vec<ForYouEntry>>>,
    fail: AtomicBool,
    definitions: AtomicU64,
    wait: AtomicBool,
    entered: Semaphore,
    gate: Semaphore,
}
impl HomeProvider {
    fn new(id: i64) -> Self {
        Self {
            common: ProviderCommonData::new(Some(id), "home", "home", "home"),
            epoch: AtomicU64::new(0),
            entries: Mutex::new(None),
            fail: AtomicBool::new(false),
            definitions: AtomicU64::new(0),
            wait: AtomicBool::new(false),
            entered: Semaphore::new(0),
            gate: Semaphore::new(0),
        }
    }
}
impl HasCommonData for HomeProvider {
    fn common(&self) -> &ProviderCommonData {
        &self.common
    }
}
impl ProviderSession for HomeProvider {
    fn load_cookie(&self, _: &str) {}
    fn save_cookie(&self) -> String {
        String::new()
    }
}
#[async_trait::async_trait]
impl Provider for HomeProvider {
    fn lifecycle_epoch(&self) -> u64 {
        self.epoch.load(Ordering::Acquire)
    }
    fn load(&self, _: &str) {}
    fn save(&self) -> String {
        String::new()
    }
    async fn check(&self, _: &Context) -> Result<(), ProviderError> {
        Ok(())
    }
    async fn auth(&self, _: &Context, _: &AuthInfo) -> Result<AuthResult, ProviderError> {
        Ok(AuthResult::Ok)
    }
    async fn sync(&self, _: &Context) -> Result<(), ProviderError> {
        Ok(())
    }
    async fn sync_item(&self, _: &Context, _: m::item::Model) -> Result<(), ProviderError> {
        Ok(())
    }
    async fn favorite(
        &self,
        _: &Context,
        _: &str,
        _: ItemType,
        _: bool,
    ) -> Result<(), ProviderError> {
        Ok(())
    }
    async fn image(
        &self,
        _: &Context,
        _: &str,
        _: Option<&str>,
        _: m::type_enum::ImageType,
    ) -> Result<reqwest::Response, ProviderError> {
        Err(ProviderError::NotImplemented)
    }
    async fn audio(
        &self,
        _: &Context,
        _: &str,
        _: Option<qcm_core::http::HeaderMap>,
    ) -> Result<reqwest::Response, ProviderError> {
        Err(ProviderError::NotImplemented)
    }
    async fn subtitle(&self, _: &str) -> Result<Subtitle, ProviderError> {
        Err(ProviderError::NotImplemented)
    }
    async fn queue_definition(
        &self,
        _: &Context,
        _: &str,
    ) -> Result<qcm_core::provider::QueueDefinition, ProviderError> {
        self.definitions.fetch_add(1, Ordering::Relaxed);
        Ok(Default::default())
    }
    async fn for_you_items(&self, _: &Context) -> Result<Vec<ForYouEntry>, ProviderError> {
        if self.wait.load(Ordering::Relaxed) {
            self.entered.add_permits(1);
            self.gate.acquire().await.unwrap().forget();
        }
        if self.fail.load(Ordering::Relaxed) {
            return Err(ProviderError::NotAuth);
        }
        self.entries
            .lock()
            .unwrap()
            .clone()
            .ok_or(ProviderError::NotImplemented)
    }
}
fn entry(key: &str, id: i64, action: CardAction) -> ForYouEntry {
    ForYouEntry {
        entry_key: key.into(),
        target: qcm_core::db::sync_batch::ItemRef::Id(id),
        presentation: Default::default(),
        primary_action: action,
        play_action: None,
    }
}

#[tokio::test]
async fn home_recommendation_contract_and_snapshot() {
    use migration::{Migrator, MigratorTrait};
    let db = sea_orm::Database::connect("sqlite::memory:").await.unwrap();
    Migrator::up(&db, Some(4)).await.unwrap();
    migration::MigrationTrait::up(
        &migration::m20261017_000001_provider_session::Migration,
        &migration::SchemaManager::new(&db),
    )
    .await
    .unwrap();
    let (ev_sender, _) = tokio::sync::mpsc::channel(8);
    let ctx = Arc::new(Context {
        db: db.clone(),
        cache_db: db,
        ev_sender,
        sync_coordinator: Default::default(),
    });
    let id = 571401;
    let provider = Arc::new(HomeProvider::new(id));
    let other = Arc::new(HomeProvider::new(id + 1));
    global::add_provider(provider.clone());
    global::add_provider(other.clone());
    m::provider::ActiveModel {
        provider_id: Set(id),
        name: Set("home".into()),
        type_: Set("home".into()),
        base_url: Set(String::new()),
        auth_method: Set(None),
        cookie: Set(String::new()),
        custom: Set(String::new()),
        session_key: Set(String::new()),
        edit_time: Set(chrono::Utc::now()),
    }
    .insert(&ctx.db)
    .await
    .unwrap();
    let writer = qcm_core::db::sync_batch::ProviderWriter::new(&ctx.db, id, &ctx.sync_coordinator);
    let batch = serde_json::from_value(serde_json::json!({"remote_mixes": [
        {"item": {"native_id":"daily","library_id":null,"type":"Mix"},"name":"daily","mix_type":"daily"},
        {"item": {"native_id":"radio/a","library_id":null,"type":"Radio"},"name":"A","mix_type":"queue"},
        {"item": {"native_id":"radio/b","library_id":null,"type":"Radio"},"name":"B","mix_type":"queue"}
    ]})).unwrap();
    let written = writer.sync_batch(batch).await.unwrap();
    let daily = written.remote_mixes[0].id;
    let radio = written.remote_mixes[1].id;
    let mix = m::mix::Entity::find()
        .filter(m::mix::Column::RemoteId.eq(daily))
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap();
    m::mix::Entity::update_many()
        .col_expr(m::mix::Column::Id, Expr::value(98765))
        .filter(m::mix::Column::Id.eq(mix.id))
        .exec(&ctx.db)
        .await
        .unwrap();
    let legacy = recommendations(&ctx, &[id]).await.providers.remove(0);
    assert!(legacy.error.is_empty(), "{}", legacy.error);
    assert_eq!(legacy.for_you.len(), 1);
    assert_eq!(legacy.radios.len(), 2);
    assert!(matches!(&legacy.for_you[0].target, Some(Target::Mix(v)) if v.id == 98765));
    let mut cards = vec![
        entry("daily", daily, CardAction::OpenItem),
        entry("radio", radio, CardAction::ActivateRadio),
    ];
    cards[0].play_action = Some(CardAction::PlayItem);
    cards[1].presentation.style = CardStyle::Radio;
    cards[1].presentation.cover = CardCover::RadioCurrent;
    *provider.entries.lock().unwrap() = Some(cards.clone());
    provider.definitions.store(0, Ordering::Relaxed);
    let valid = recommendations(&ctx, &[id]).await.providers.remove(0);
    assert!(valid.error.is_empty(), "{}", valid.error);
    assert_eq!(valid.for_you.len(), 2);
    assert_eq!(valid.radios.len(), 1);
    assert_eq!(provider.definitions.load(Ordering::Relaxed), 2);
    assert_eq!(valid.for_you[0].cover.as_ref().unwrap().id, 98765);
    assert_eq!(radio_queues(&ctx, &[id]).await.unwrap().len(), 2);
    let mut duplicate_target = cards.clone();
    duplicate_target.push(entry("another", daily, CardAction::PlayItem));
    *provider.entries.lock().unwrap() = Some(duplicate_target);
    assert_eq!(
        recommendations(&ctx, &[id]).await.providers[0]
            .for_you
            .len(),
        3
    );
    let mut missing = cards.clone();
    missing[0].target = qcm_core::db::sync_batch::ItemRef::Id(9999999);
    *provider.entries.lock().unwrap() = Some(missing);
    let missing = recommendations(&ctx, &[id]).await.providers.remove(0);
    assert!(missing.error.is_empty());
    assert_eq!(missing.for_you.len(), 1);
    assert_eq!(missing.diagnostics.len(), 1);
    assert_eq!(missing.radios.len(), 1);
    let mut bad = cards.clone();
    bad[1].primary_action = CardAction::PlayItem;
    *provider.entries.lock().unwrap() = Some(bad);
    assert!(!recommendations(&ctx, &[id]).await.providers[0]
        .error
        .is_empty());
    let mut bad = cards.clone();
    bad[1].entry_key = bad[0].entry_key.clone();
    *provider.entries.lock().unwrap() = Some(bad);
    assert!(!recommendations(&ctx, &[id]).await.providers[0]
        .error
        .is_empty());
    let mut missing_library = cards.clone();
    missing_library[0].target = serde_json::from_value(
        serde_json::json!({"native_id":"daily", "library_id":99999, "type":"Mix"}),
    )
    .unwrap();
    *provider.entries.lock().unwrap() = Some(missing_library);
    let missing = recommendations(&ctx, &[id]).await.providers.remove(0);
    assert!(missing.error.is_empty());
    assert_eq!(missing.for_you.len(), 1);
    assert_eq!(missing.diagnostics.len(), 1);
    *other.entries.lock().unwrap() = Some(vec![entry("foreign", daily, CardAction::OpenItem)]);
    assert!(!recommendations(&ctx, &[id + 1]).await.providers[0]
        .error
        .is_empty());
    *other.entries.lock().unwrap() = None;
    *provider.entries.lock().unwrap() = Some(vec![]);
    let empty = recommendations(&ctx, &[id]).await.providers.remove(0);
    assert!(empty.for_you.is_empty());
    assert_eq!(empty.radios.len(), 2);
    provider.fail.store(true, Ordering::Relaxed);
    let partial = recommendations(&ctx, &[id, id + 1]).await;
    assert!(!partial.providers[0].error.is_empty());
    assert!(partial.providers[1].error.is_empty());
    provider.fail.store(false, Ordering::Relaxed);
    provider.wait.store(true, Ordering::Relaxed);
    let task_ctx = ctx.clone();
    let task = tokio::spawn(async move { recommendations(&task_ctx, &[id]).await });
    provider.entered.acquire().await.unwrap().forget();
    provider.epoch.fetch_add(1, Ordering::Relaxed);
    provider.gate.add_permits(1);
    assert!(!task.await.unwrap().providers[0].error.is_empty());
    let task_ctx = ctx.clone();
    let task = tokio::spawn(async move { recommendations(&task_ctx, &[id]).await });
    provider.entered.acquire().await.unwrap().forget();
    writer.sync_batch(Default::default()).await.unwrap();
    provider.gate.add_permits(1);
    assert!(!task.await.unwrap().providers[0].error.is_empty());
    let old = global::provider_snapshot(id).unwrap().1;
    global::add_provider(Arc::new(HomeProvider::new(id)));
    assert_ne!(global::provider_snapshot(id).unwrap().1, old);
    global::remove_provider(id);
    global::remove_provider(id + 1);
}
