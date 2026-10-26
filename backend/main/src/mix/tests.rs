use super::*;
use crate::{api::process_qcm, event::ServiceContext, msg};
use prost::Message;
use qcm_core::{
    db::values::Timestamp,
    error::ProviderError,
    model as m,
    provider::{
        AuthInfo, AuthResult, HasCommonData, Provider, ProviderCommonData, ProviderSession,
    },
    subtitle::Subtitle,
};
use sea_orm::{ConnectOptions, ConnectionTrait, Database, EntityTrait, Schema};
use std::{
    path::PathBuf,
    sync::atomic::{AtomicBool, AtomicI64, AtomicUsize, Ordering},
};
use tokio::sync::Notify;

const MIX: i64 = 91;
const REMOTE: i64 = 501;
static NEXT_PROVIDER: AtomicI64 = AtomicI64::new(819100);

struct MixProvider {
    common: ProviderCommonData,
    calls: AtomicUsize,
    fail: AtomicBool,
    empty: AtomicBool,
    wait: AtomicBool,
    entered: Notify,
    release: Notify,
}
impl HasCommonData for MixProvider {
    fn common(&self) -> &ProviderCommonData {
        &self.common
    }
}
impl ProviderSession for MixProvider {
    fn load_cookie(&self, _: &str) {}
    fn save_cookie(&self) -> String {
        String::new()
    }
}
#[async_trait::async_trait]
impl Provider for MixProvider {
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
    async fn sync_item(&self, ctx: &Context, item: m::item::Model) -> Result<(), ProviderError> {
        assert_eq!(
            item.id, REMOTE,
            "local mix ID was not resolved to its remote item"
        );
        assert_eq!(item.native_id, "daily");
        self.calls.fetch_add(1, Ordering::Relaxed);
        if self.wait.swap(false, Ordering::Relaxed) {
            self.entered.notify_one();
            self.release.notified().await;
        }
        if self.fail.swap(false, Ordering::Relaxed) {
            return Err(ProviderError::NotAuth);
        }
        let _write = ctx.sync_coordinator.write(item.provider_id, None).await?;
        ctx.db
            .execute_unprepared("DELETE FROM rel_mix_song WHERE mix_id=91;")
            .await?;
        let count = if self.empty.load(Ordering::Relaxed) {
            0
        } else {
            ctx.db.execute_unprepared("INSERT INTO rel_mix_song(mix_id,song_id,order_idx,update_at) VALUES(91,503,0,0);").await?;
            1
        };
        ctx.db
            .execute_unprepared(&format!(
            "UPDATE mix SET name='Refreshed',track_count={count},content_update_at={} WHERE id=91;",
            Timestamp::now().as_millis()
        ))
            .await?;
        Ok(())
    }
    async fn favorite(
        &self,
        _: &Context,
        _: &str,
        _: m::type_enum::ItemType,
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
}

struct TestContext {
    service: Arc<ServiceContext>,
    provider: Arc<MixProvider>,
    directory: PathBuf,
    id: i64,
}
impl TestContext {
    async fn new(fresh: bool) -> Self {
        let id = NEXT_PROVIDER.fetch_add(1, Ordering::Relaxed);
        let mut options = ConnectOptions::new("sqlite::memory:");
        options.max_connections(1);
        let db = Database::connect(options).await.unwrap();
        db.execute_unprepared("PRAGMA foreign_keys=OFF")
            .await
            .unwrap();
        let schema = Schema::new(db.get_database_backend());
        for table in [
            schema.create_table_from_entity(m::item::Entity),
            schema.create_table_from_entity(m::song::Entity),
            schema.create_table_from_entity(m::album::Entity),
            schema.create_table_from_entity(m::artist::Entity),
            schema.create_table_from_entity(m::dynamic::Entity),
            schema.create_table_from_entity(m::mix::Entity),
            schema.create_table_from_entity(m::remote_mix::Entity),
            schema.create_table_from_entity(m::rel_mix_song::Entity),
            schema.create_table_from_entity(m::rel_song_artist::Entity),
            schema.create_table_from_entity(m::rel_album_artist::Entity),
        ] {
            db.execute(db.get_database_backend().build(&table))
                .await
                .unwrap();
        }
        let updated = if fresh {
            Timestamp::now().as_millis()
        } else {
            0
        };
        db.execute_unprepared(&format!(
            "INSERT INTO item(id,native_id,provider_id,type,create_at,update_at,last_sync_at) VALUES
             (501,'daily',{id},54,0,0,0),(502,'old',{id},101,0,0,0),(503,'new',{id},101,0,0,0);
             INSERT INTO song(id,name,disc_number,track_number,duration,can_play,popularity,tags) VALUES
             (502,'Old',1,1,1000,1,0,'[]'),(503,'New',1,1,1000,1,0,'[]');
             INSERT INTO remote_mix(id,name,description,track_count,mix_type) VALUES(501,'Daily','',1,'daily');
             INSERT INTO mix(id,name,description,track_count,mix_type,remote_id,content_update_at) VALUES
             (91,'Daily','',1,2,501,{updated}),(92,'Local','',1,0,NULL,0);
             INSERT INTO rel_mix_song(mix_id,song_id,order_idx,update_at) VALUES(91,502,0,0),(92,502,0,0);"
        )).await.unwrap();
        let provider = Arc::new(MixProvider {
            common: ProviderCommonData::new(Some(id), "mix", "mix", "mix"),
            calls: AtomicUsize::new(0),
            fail: AtomicBool::new(false),
            empty: AtomicBool::new(false),
            wait: AtomicBool::new(false),
            entered: Notify::new(),
            release: Notify::new(),
        });
        global::add_provider(provider.clone());
        let directory =
            std::env::temp_dir().join(format!("qcm-mix-refresh-{}", uuid::Uuid::new_v4()));
        let cache = crate::resource_cache::ResourceCache::open(directory.clone())
            .await
            .unwrap();
        let (events, _) = tokio::sync::mpsc::channel(8);
        let (backend_ev, _) = tokio::sync::mpsc::channel(8);
        let (oper, _) = crate::task::TaskManager::new();
        let service = Arc::new(ServiceContext {
            provider_context: Arc::new(Context {
                db: db.clone(),
                cache_db: db,
                ev_sender: events,
                sync_coordinator: Default::default(),
            }),
            backend_ev,
            oper,
            resource_cache: cache,
            mix_refresh: Default::default(),
            playback: Default::default(),
            radio: Default::default(),
        });
        Self {
            service,
            provider,
            directory,
            id,
        }
    }
    async fn songs(&self, id: i64, force_refresh: bool) -> Result<msg::QcmMessage, ProcessError> {
        songs(&self.service, id, force_refresh).await
    }
    fn calls(&self) -> usize {
        self.provider.calls.load(Ordering::Relaxed)
    }
    async fn finish(self) {
        self.service.resource_cache.shutdown().await.unwrap();
        global::remove_provider(self.id);
        std::fs::remove_dir_all(self.directory).unwrap();
    }
}
async fn songs(
    service: &Arc<ServiceContext>,
    id: i64,
    force_refresh: bool,
) -> Result<msg::QcmMessage, ProcessError> {
    let request = msg::QcmMessage {
        r#type: msg::MessageType::GetMixSongsReq as i32,
        payload: Some(msg::qcm_message::Payload::GetMixSongsReq(
            msg::GetMixSongsReq {
                id,
                force_refresh,
                page_size: 20,
                sort_asc: true,
                ..Default::default()
            },
        )),
        ..Default::default()
    };
    process_qcm(service, &request.encode_to_vec(), &mut None).await
}
fn ids(response: &msg::QcmMessage) -> &[i64] {
    let Some(msg::qcm_message::Payload::GetMixSongsRsp(value)) = &response.payload else {
        panic!("expected mix songs");
    };
    &value.ids
}

#[tokio::test]
async fn fresh_mix_is_cached_and_manual_refresh_uses_remote_item_id() {
    let ctx = TestContext::new(true).await;
    assert_eq!(ids(&ctx.songs(MIX, false).await.unwrap()), &[502]);
    assert_eq!(ctx.calls(), 0);
    let refreshed = ctx.songs(MIX, true).await.unwrap();
    assert_eq!(ids(&refreshed), &[503]);
    assert_eq!(ctx.calls(), 1);
    assert_eq!(
        refreshed.entities.unwrap().mixes[0]
            .fields
            .as_ref()
            .unwrap()
            .name
            .as_deref(),
        Some("Refreshed")
    );
    assert_eq!(ids(&ctx.songs(MIX, false).await.unwrap()), &[503]);
    assert_eq!(ctx.calls(), 1);
    ctx.finish().await;
}

#[tokio::test]
async fn expired_mix_refreshes_before_query_and_normal_mix_stays_local() {
    let ctx = TestContext::new(false).await;
    assert_eq!(ids(&ctx.songs(MIX, false).await.unwrap()), &[503]);
    assert_eq!(ctx.calls(), 1);
    assert_eq!(ids(&ctx.songs(92, true).await.unwrap()), &[502]);
    assert_eq!(ctx.calls(), 1);
    ctx.finish().await;
}

#[tokio::test]
async fn filtered_playback_ids_refresh_expired_mix_membership() {
    let ctx = TestContext::new(true).await;
    let expired = Timestamp::now().as_millis() - 31 * 60 * 1000;
    ctx.service
        .provider_context
        .db
        .execute_unprepared(&format!(
            "UPDATE mix SET content_update_at={expired} WHERE id=91;"
        ))
        .await
        .unwrap();
    let request = msg::QcmMessage {
        r#type: msg::MessageType::GetSongIdsReq as i32,
        payload: Some(msg::qcm_message::Payload::GetSongIdsReq(
            msg::GetSongIdsReq {
                sort: msg::model::SongSort::Title as i32,
                asc: true,
                filters: vec![msg::filter::SongFilter {
                    payload: Some(msg::filter::song_filter::Payload::MixIdFilter(
                        msg::filter::MixIdFilter { value: MIX },
                    )),
                    ..Default::default()
                }],
                ..Default::default()
            },
        )),
        ..Default::default()
    };
    let response = process_qcm(&ctx.service, &request.encode_to_vec(), &mut None)
        .await
        .unwrap();
    let Some(msg::qcm_message::Payload::GetSongIdsRsp(response)) = response.payload else {
        panic!("expected playback IDs");
    };
    assert_eq!(response.ids, vec![503]);
    assert_eq!(ctx.calls(), 1);
    assert_eq!(ids(&ctx.songs(MIX, false).await.unwrap()), &[503]);
    assert_eq!(ctx.calls(), 1);
    ctx.finish().await;
}

#[tokio::test]
async fn failed_refresh_keeps_old_membership_and_expiry_for_retry() {
    let ctx = TestContext::new(false).await;
    ctx.provider.fail.store(true, Ordering::Relaxed);
    assert!(ctx.songs(MIX, false).await.is_err());
    let mix = m::mix::Entity::find_by_id(MIX)
        .one(&ctx.service.provider_context.db)
        .await
        .unwrap()
        .unwrap();
    assert_eq!(mix.content_update_at.as_millis(), 0);
    let members = m::rel_mix_song::Entity::find()
        .all(&ctx.service.provider_context.db)
        .await
        .unwrap();
    assert!(members
        .iter()
        .any(|value| value.mix_id == MIX && value.song_id == 502));
    assert_eq!(ids(&ctx.songs(MIX, false).await.unwrap()), &[503]);
    assert_eq!(ctx.calls(), 2);
    ctx.finish().await;
}

#[tokio::test]
async fn failed_manual_refresh_keeps_fresh_cache_and_can_be_retried() {
    let ctx = TestContext::new(true).await;
    ctx.provider.fail.store(true, Ordering::Relaxed);
    assert!(ctx.songs(MIX, true).await.is_err());
    assert_eq!(ids(&ctx.songs(MIX, false).await.unwrap()), &[502]);
    assert_eq!(ctx.calls(), 1);
    assert_eq!(ids(&ctx.songs(MIX, true).await.unwrap()), &[503]);
    assert_eq!(ctx.calls(), 2);
    ctx.finish().await;
}

#[tokio::test]
async fn empty_refresh_is_fresh_and_returns_an_empty_list() {
    let ctx = TestContext::new(false).await;
    ctx.provider.empty.store(true, Ordering::Relaxed);
    assert!(ids(&ctx.songs(MIX, false).await.unwrap()).is_empty());
    assert!(ids(&ctx.songs(MIX, false).await.unwrap()).is_empty());
    assert_eq!(ctx.calls(), 1);
    ctx.finish().await;
}

#[tokio::test]
async fn concurrent_expired_reads_share_one_remote_sync() {
    let ctx = TestContext::new(false).await;
    ctx.provider.wait.store(true, Ordering::Relaxed);
    let first_service = ctx.service.clone();
    let first = tokio::spawn(async move { songs(&first_service, MIX, false).await });
    ctx.provider.entered.notified().await;
    let second_service = ctx.service.clone();
    let mut second = tokio::spawn(async move { songs(&second_service, MIX, false).await });
    assert!(
        tokio::time::timeout(std::time::Duration::from_millis(50), &mut second)
            .await
            .is_err()
    );
    assert_eq!(ctx.calls(), 1);
    ctx.provider.release.notify_one();
    assert_eq!(ids(&first.await.unwrap().unwrap()), &[503]);
    assert_eq!(ids(&second.await.unwrap().unwrap()), &[503]);
    assert_eq!(ctx.calls(), 1);
    ctx.finish().await;
}
