use super::*;
use qcm_core::{
    model::{
        item,
        type_enum::{ImageType, ItemType},
    },
    provider::{AuthInfo, AuthResult, HasCommonData, ProviderCommonData, ProviderSession},
    subtitle::Subtitle,
};
use std::sync::atomic::AtomicU64;
use tokio::sync::Semaphore;
mod sync_tests;

struct TestProvider {
    common: ProviderCommonData,
    epoch: AtomicU64,
    opens: AtomicU64,
    nexts: AtomicU64,
    fail: AtomicBool,
    closed: Mutex<Vec<String>>,
    songs: Mutex<Vec<i64>>,
    source: Mutex<Option<i64>>,
    media: Mutex<Option<String>>,
    entered: Semaphore,
    gate: Semaphore,
    delay: AtomicBool,
}
impl Default for TestProvider {
    fn default() -> Self {
        Self {
            common: ProviderCommonData::new(Some(1), "test", "test", "test"),
            epoch: AtomicU64::new(0),
            opens: AtomicU64::new(0),
            nexts: AtomicU64::new(0),
            fail: AtomicBool::new(false),
            songs: Mutex::new(vec![7]),
            source: Mutex::new(None),
            media: Mutex::new(None),
            closed: Mutex::new(vec![]),
            entered: Semaphore::new(0),
            gate: Semaphore::new(0),
            delay: AtomicBool::new(false),
        }
    }
}
impl HasCommonData for TestProvider {
    fn common(&self) -> &ProviderCommonData {
        &self.common
    }
}
impl ProviderSession for TestProvider {
    fn load_cookie(&self, _: &str) {}
    fn save_cookie(&self) -> String {
        String::new()
    }
}
fn batch(state: &str) -> QueueBatch {
    QueueBatch { state: state.into(), songs: vec![serde_json::from_value(serde_json::json!({
        "id": 7, "name": "song", "album_id": null, "track_number": 1, "disc_number": 1, "duration": 1000,
    })).unwrap()], ..Default::default() }
}
impl TestProvider {
    fn recommendation(&self, state: &str) -> QueueBatch {
        let mut result = batch(state);
        result.presentation.source_item_id = *self.source.lock().unwrap();
        let song = result.songs[0].clone();
        result.songs = self.songs.lock().unwrap().iter().map(|id| {
            let mut song = song.clone();
            song.id = *id;
            song
        }).collect();
        result
    }
}
#[async_trait::async_trait]
impl Provider for TestProvider {
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
    async fn sync_item(&self, _: &Context, _: item::Model) -> Result<(), ProviderError> {
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
        item: &str,
        image: Option<&str>,
        _: ImageType,
    ) -> Result<reqwest::Response, ProviderError> {
        let base = self.media.lock().unwrap().clone().ok_or(ProviderError::NotImplemented)?;
        Ok(reqwest::get(format!("{base}/image/{item}/{}", image.unwrap_or_default())).await?)
    }
    async fn audio(
        &self,
        _: &Context,
        item: &str,
        _: Option<qcm_core::http::HeaderMap>,
    ) -> Result<reqwest::Response, ProviderError> {
        let base = self.media.lock().unwrap().clone().ok_or(ProviderError::NotImplemented)?;
        Ok(reqwest::get(format!("{base}/audio/{item}")).await?)
    }
    async fn subtitle(&self, _: &str) -> Result<Subtitle, ProviderError> {
        Err(ProviderError::NotImplemented)
    }
    async fn open_queue(
        &self,
        _: &Context,
        _: &str,
        req: QueueOpenRequest,
    ) -> Result<QueueBatch, ProviderError> {
        self.opens.fetch_add(1, Ordering::AcqRel);
        if self.delay.swap(false, Ordering::AcqRel) {
            self.entered.add_permits(1);
            self.gate.acquire().await.unwrap().forget();
        }
        if req.configuration.contains_key("fail") {
            return Err(ProviderError::NotFound);
        }
        let mut result = self.recommendation(req.previous_state.as_deref().unwrap_or("0"));
        if let Some(input) = req.input {
            assert_eq!(input.native_id, "native-9");
            assert_eq!(input.item_type, ItemType::Song);
            result.presentation.source_item_id = Some(9);
        }
        Ok(result)
    }
    async fn next_queue(
        &self,
        _: &Context,
        _: &str,
        req: QueueNextRequest,
    ) -> Result<QueueBatch, ProviderError> {
        self.nexts.fetch_add(1, Ordering::AcqRel);
        if self.delay.swap(false, Ordering::AcqRel) {
            self.entered.add_permits(1);
            self.gate.acquire().await.unwrap().forget();
        }
        if self.fail.swap(false, Ordering::AcqRel) {
            return Err(ProviderError::NotFound);
        }
        Ok(self.recommendation(&(req.state.parse::<u32>().unwrap() + 1).to_string()))
    }
    async fn close_queue(&self, _: &Context, _: &str, state: &str) -> Result<(), ProviderError> {
        self.closed.lock().unwrap().push(state.into());
        Ok(())
    }
}
fn operation_id() -> String {
    format!(
        "{}:{}",
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_millis(),
        uuid::Uuid::new_v4()
    )
}
async fn setup() -> (Arc<RadioSessions>, Arc<Context>, Target, Arc<TestProvider>) {
    setup_provider(1).await
}
async fn setup_provider(provider_id: i64) -> (Arc<RadioSessions>, Arc<Context>, Target, Arc<TestProvider>) {
    use qcm_core::{model as m, provider::ProviderCommon};
    use sea_orm::{ConnectionTrait, EntityTrait, IntoActiveModel, Schema};
    let owner = Arc::new(RadioSessions {
        validate_provider: Arc::new(|_| true),
        ..Default::default()
    });
    let db = sea_orm::Database::connect("sqlite::memory:").await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=OFF").await.unwrap();
    let schema = Schema::new(db.get_database_backend());
    for statement in [
        schema.create_table_from_entity(m::item::Entity),
        schema.create_table_from_entity(m::song::Entity),
        schema.create_table_from_entity(m::artist::Entity),
        schema.create_table_from_entity(m::album::Entity),
        schema.create_table_from_entity(m::dynamic::Entity),
        schema.create_table_from_entity(m::rel_song_artist::Entity),
        schema.create_table_from_entity(m::rel_album_artist::Entity),
    ] {
        db.execute(db.get_database_backend().build(&statement)).await.unwrap();
    }
    for (id, item_type) in [(5, ItemType::Radio), (7, ItemType::Song),
        (9, ItemType::Song), (11, ItemType::Song)] {
        let model: m::item::Model = serde_json::from_value(serde_json::json!({
            "id":id,"native_id":format!("native-{id}"),"provider_id":if id == 11 {provider_id+1} else {provider_id},
            "library_id":null,"type":item_type,
        })).unwrap();
        m::item::Entity::insert(model.into_active_model()).exec(&db).await.unwrap();
    }
    for id in [7, 9] {
        let mut song = batch("private").songs.remove(0);
        song.id = id;
        m::song::Entity::insert(song.into_active_model()).exec(&db).await.unwrap();
    }
    let (ev_sender, _) = tokio::sync::mpsc::channel(1);
    let ctx = Arc::new(Context {
        db: db.clone(),
        cache_db: db,
        ev_sender,
        sync_coordinator: Default::default(),
    });
    let provider = Arc::new(TestProvider::default());
    provider.set_id(Some(provider_id));
    let target = Target {
        queue_id: 5,
        native_id: "radio".into(),
        provider: provider.clone(),
    };
    (owner, ctx, target, provider)
}
async fn open(owner: &Arc<RadioSessions>, ctx: &Arc<Context>, target: &Target) -> Page {
    owner
        .open(
            ctx.clone(),
            target.clone(),
            operation_id(),
            String::new(),
            Default::default(),
        )
        .await
        .unwrap()
}

#[tokio::test]
async fn lost_response_replays_without_opening_or_advancing_twice() {
    let (owner, ctx, target, provider) = setup().await;
    let op = operation_id();
    let first = owner
        .open(
            ctx.clone(),
            target.clone(),
            op.clone(),
            String::new(),
            Default::default(),
        )
        .await
        .unwrap();
    let replay = owner
        .open(
            ctx.clone(),
            target.clone(),
            op,
            String::new(),
            Default::default(),
        )
        .await
        .unwrap();
    assert_eq!(first.session_id, replay.session_id);
    assert_eq!(provider.opens.load(Ordering::Acquire), 1);
    let op = operation_id();
    let next = owner
        .next(
            ctx.clone(),
            target.clone(),
            first.session_id.clone(),
            op.clone(),
            1,
            vec![],
        )
        .await
        .unwrap();
    let replay = owner
        .next(
            ctx.clone(),
            target.clone(),
            first.session_id.clone(),
            op.clone(),
            1,
            vec![],
        )
        .await
        .unwrap();
    assert_eq!(next.version, 2);
    assert_eq!(replay.batch.state, "1");
    assert_eq!(provider.nexts.load(Ordering::Acquire), 1);
    assert!(matches!(
        owner
            .next(
                ctx.clone(),
                target.clone(),
                first.session_id.clone(),
                op,
                1,
                vec!["other".into()]
            )
            .await,
        Err(RadioError::Conflict)
    ));
    assert!(matches!(
        owner
            .next(
                ctx.clone(),
                target.clone(),
                first.session_id,
                operation_id(),
                1,
                vec![]
            )
            .await,
        Err(RadioError::Conflict)
    ));
}

#[tokio::test]
async fn in_flight_retries_survive_request_cancellation() {
    let (owner, ctx, target, provider) = setup().await;
    provider.delay.store(true, Ordering::Release);
    let op = operation_id();
    let task = tokio::spawn({
        let owner = owner.clone();
        let ctx = ctx.clone();
        let target = target.clone();
        let op = op.clone();
        async move {
            owner
                .open(ctx, target, op, String::new(), Default::default())
                .await
        }
    });
    provider.entered.acquire().await.unwrap().forget();
    task.abort();
    let retry = tokio::spawn({
        let owner = owner.clone();
        let ctx = ctx.clone();
        let target = target.clone();
        async move {
            owner
                .open(ctx, target, op, String::new(), Default::default())
                .await
        }
    });
    provider.gate.add_permits(1);
    assert!(retry.await.unwrap().is_ok());
    assert_eq!(provider.opens.load(Ordering::Acquire), 1);
}

#[tokio::test]
async fn failure_and_candidate_do_not_mutate_current_session() {
    let (owner, ctx, target, provider) = setup().await;
    let first = open(&owner, &ctx, &target).await;
    provider.fail.store(true, Ordering::Release);
    assert!(owner
        .next(
            ctx.clone(),
            target.clone(),
            first.session_id.clone(),
            operation_id(),
            1,
            vec![]
        )
        .await
        .is_err());
    let failure = QueueOpenRequest {
        configuration: HashMap::from([("fail".into(), "1".into())]),
        ..Default::default()
    };
    assert!(owner
        .open(
            ctx.clone(),
            target.clone(),
            operation_id(),
            first.session_id.clone(),
            failure
        )
        .await
        .is_err());
    let next = owner
        .next(
            ctx.clone(),
            target.clone(),
            first.session_id.clone(),
            operation_id(),
            1,
            vec![],
        )
        .await
        .unwrap();
    assert_eq!(next.batch.state, "1");
    let candidate = owner
        .open(
            ctx.clone(),
            target.clone(),
            operation_id(),
            first.session_id.clone(),
            Default::default(),
        )
        .await
        .unwrap();
    assert_eq!(candidate.batch.state, "1");
    owner.close(&ctx, &candidate.session_id).await;
    owner.close(&ctx, &candidate.session_id).await;
    assert_eq!(provider.closed.lock().unwrap().len(), 1);
    assert!(owner
        .next(
            ctx.clone(),
            target,
            first.session_id,
            operation_id(),
            2,
            vec![]
        )
        .await
        .is_ok());
}

#[tokio::test]
async fn handles_are_bound_and_provider_epoch_expires_them() {
    let (owner, ctx, target, provider) = setup().await;
    let page = open(&owner, &ctx, &target).await;
    let other_client = Arc::new(RadioSessions::default());
    assert!(matches!(
        other_client
            .next(
                ctx.clone(),
                target.clone(),
                page.session_id.clone(),
                operation_id(),
                1,
                vec![]
            )
            .await,
        Err(RadioError::Expired)
    ));
    let mut other_queue = target.clone();
    other_queue.queue_id += 1;
    assert!(matches!(
        owner
            .next(
                ctx.clone(),
                other_queue,
                page.session_id.clone(),
                operation_id(),
                1,
                vec![]
            )
            .await,
        Err(RadioError::Expired)
    ));
    provider.epoch.fetch_add(1, Ordering::AcqRel);
    assert!(matches!(
        owner
            .next(
                ctx.clone(),
                target,
                page.session_id,
                operation_id(),
                1,
                vec![]
            )
            .await,
        Err(RadioError::Expired)
    ));
    owner.collect(&ctx).await;
    assert_eq!(provider.closed.lock().unwrap().len(), 1);
}

#[tokio::test]
async fn provider_change_during_open_releases_rejected_candidate() {
    let (owner, ctx, target, provider) = setup().await;
    provider.delay.store(true, Ordering::Release);
    let task = tokio::spawn({
        let owner = owner.clone();
        let ctx = ctx.clone();
        async move {
            owner
                .open(
                    ctx,
                    target,
                    operation_id(),
                    String::new(),
                    Default::default(),
                )
                .await
        }
    });
    provider.entered.acquire().await.unwrap().forget();
    provider.epoch.fetch_add(1, Ordering::AcqRel);
    provider.gate.add_permits(1);
    assert!(matches!(task.await.unwrap(), Err(RadioError::Expired)));
    assert_eq!(provider.closed.lock().unwrap().len(), 1);
}

#[test]
fn expired_operation_cannot_start_a_new_recommendation() {
    assert!(matches!(
        check_operation_id(&format!("1:{}", uuid::Uuid::new_v4())),
        Err(RadioError::Expired)
    ));
}

#[tokio::test]
async fn protobuf_dispatch_keeps_song_order_and_private_state_on_backend() {
    use crate::{event::ServiceContext, msg, task::TaskManager};
    use prost::Message;
    let provider_id = 872146;
    let (_, ctx, mut target, provider) = setup_provider(provider_id).await;
    *provider.songs.lock().unwrap() = vec![9, 7, 9];
    target.queue_id = 5;
    qcm_core::global::add_provider(provider.clone());
    let directory = std::env::temp_dir().join(format!("qcm-radio-{}", uuid::Uuid::new_v4()));
    let cache = crate::resource_cache::ResourceCache::open(directory.clone())
        .await
        .unwrap();
    let (oper, _) = TaskManager::new();
    let (backend_ev, _) = tokio::sync::mpsc::channel(1);
    let service = Arc::new(ServiceContext {
        provider_context: ctx.clone(),
        backend_ev,
        oper,
        resource_cache: cache.clone(),
        radio: Arc::new(Default::default()),
    });
    let request = msg::QcmMessage {
        id: 17,
        r#type: msg::MessageType::OpenQueueReq.into(),
        payload: Some(msg::qcm_message::Payload::OpenQueueReq(msg::OpenQueueReq {
            queue_id: 5,
            input_id: 9,
            operation_id: operation_id(),
            ..Default::default()
        })),
    };
    let response = crate::api::process_qcm(&service, &request.encode_to_vec(), &mut None)
        .await
        .unwrap();
    let decoded = msg::QcmMessage::decode(response.encode_to_vec().as_slice()).unwrap();
    let Some(msg::qcm_message::Payload::OpenQueueRsp(page)) = decoded.payload else {
        panic!("wrong response");
    };
    assert_eq!(
        page.songs.iter().map(|s| s.id).collect::<Vec<_>>(),
        vec![9, 7]
    );
    assert_eq!(page.extras.len(), 2);
    assert_eq!(page.session.as_ref().unwrap().version, 1);
    assert_eq!(
        page.session
            .as_ref()
            .unwrap()
            .presentation
            .as_ref()
            .unwrap()
            .source_item_id,
        9
    );
    let mut rejected = request.clone();
    let Some(msg::qcm_message::Payload::OpenQueueReq(ref mut req)) = rejected.payload else {
        panic!("wrong request");
    };
    req.input_id = 11;
    req.operation_id = operation_id();
    assert!(matches!(
        crate::api::process_qcm(&service, &rejected.encode_to_vec(), &mut None).await,
        Err(crate::error::ProcessError::WrongId(_))
    ));
    assert_eq!(provider.opens.load(Ordering::Acquire), 1);
    let handle = page.session.unwrap().id;
    assert!(!handle.is_empty());
    service.radio.close(&ctx, &handle).await;
    cache.shutdown().await.unwrap();
    qcm_core::global::remove_provider(provider_id);
    let next_request = msg::QcmMessage {
        id: 18,
        r#type: msg::MessageType::GetQueueNextReq.into(),
        payload: Some(msg::qcm_message::Payload::GetQueueNextReq(
            msg::GetQueueNextReq {
                queue_id: 5,
                session_id: handle,
                operation_id: operation_id(),
                expected_version: 1,
                ..Default::default()
            },
        )),
    };
    assert!(matches!(
        crate::api::process_qcm(&service, &next_request.encode_to_vec(), &mut None).await,
        Err(crate::error::ProcessError::Radio(RadioError::Expired))
    ));
    std::fs::remove_dir_all(directory).unwrap();
}

#[tokio::test]
async fn continuation_is_serial_and_close_invalidates_before_cleanup() {
    let (owner, ctx, target, provider) = setup().await;
    let first = open(&owner, &ctx, &target).await;
    provider.delay.store(true, Ordering::Release);
    let next = tokio::spawn({
        let owner = owner.clone();
        let ctx = ctx.clone();
        let target = target.clone();
        let id = first.session_id.clone();
        async move { owner.next(ctx, target, id, operation_id(), 1, vec![]).await }
    });
    provider.entered.acquire().await.unwrap().forget();
    let following = tokio::spawn({
        let owner = owner.clone();
        let ctx = ctx.clone();
        let target = target.clone();
        let id = first.session_id.clone();
        async move { owner.next(ctx, target, id, operation_id(), 2, vec![]).await }
    });
    provider.gate.add_permits(1);
    assert_eq!(next.await.unwrap().unwrap().batch.state, "1");
    assert_eq!(following.await.unwrap().unwrap().batch.state, "2");
    provider.delay.store(true, Ordering::Release);
    let next = tokio::spawn({
        let owner = owner.clone();
        let ctx = ctx.clone();
        let target = target.clone();
        let id = first.session_id.clone();
        async move { owner.next(ctx, target, id, operation_id(), 3, vec![]).await }
    });
    provider.entered.acquire().await.unwrap().forget();
    let close = tokio::spawn({
        let owner = owner.clone();
        let ctx = ctx.clone();
        let id = first.session_id.clone();
        async move { owner.close(&ctx, &id).await }
    });
    tokio::task::yield_now().await;
    assert!(matches!(
        owner
            .next(ctx, target, first.session_id, operation_id(), 3, vec![])
            .await,
        Err(RadioError::Expired)
    ));
    provider.gate.add_permits(1);
    assert!(matches!(next.await.unwrap(), Err(RadioError::Expired)));
    close.await.unwrap();
    assert_eq!(*provider.closed.lock().unwrap(), vec!["3"]);
}

#[tokio::test]
async fn cached_failures_do_not_cross_account_epochs() {
    let (owner, ctx, target, provider) = setup().await;
    let operation = operation_id();
    let request = QueueOpenRequest {
        configuration: HashMap::from([("fail".into(), "1".into())]),
        ..Default::default()
    };
    assert!(matches!(
        owner
            .open(
                ctx.clone(),
                target.clone(),
                operation.clone(),
                String::new(),
                request.clone()
            )
            .await,
        Err(RadioError::Provider(_))
    ));
    provider.epoch.fetch_add(1, Ordering::AcqRel);
    assert!(matches!(
        owner
            .open(ctx, target, operation, String::new(), request)
            .await,
        Err(RadioError::Expired)
    ));
    assert_eq!(provider.opens.load(Ordering::Acquire), 1);
}
