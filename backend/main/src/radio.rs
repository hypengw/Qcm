use qcm_core::{
    db::retention::ItemRetention,
    error::ProviderError,
    provider::{Context, Provider, QueueBatch, QueueNextRequest, QueueOpenRequest},
};
use std::{
    collections::{BTreeMap, HashMap},
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc, Mutex,
    },
    time::{Duration, Instant, SystemTime, UNIX_EPOCH},
};
use tokio::sync::{watch, Mutex as AsyncMutex};

const MAX_SESSIONS: usize = 128;
const MAX_OPERATIONS: usize = 512;
const SESSION_TTL: Duration = Duration::from_secs(30 * 60);
const OPERATION_TTL: Duration = Duration::from_secs(5 * 60);
const MAX_STATE_BYTES: usize = 64 * 1024;

type RadioResult = Result<Page, RadioError>;
type ProviderValidator = dyn Fn(&Arc<dyn Provider>) -> bool + Send + Sync;

#[derive(Clone, Debug, thiserror::Error)]
pub enum RadioError {
    #[error("Radio session expired; restart the radio to continue")]
    Expired,
    #[error("Radio session version or operation conflict")]
    Conflict,
    #[error("{0}")]
    Invalid(String),
    #[error("{0}")]
    Provider(#[from] Arc<ProviderError>),
}
impl From<ProviderError> for RadioError {
    fn from(error: ProviderError) -> Self {
        Self::Provider(Arc::new(error))
    }
}

#[derive(Clone, Debug)]
pub struct Page {
    pub session_id: String,
    pub version: u64,
    pub batch: QueueBatch,
}

#[derive(Clone)]
pub struct Target {
    pub queue_id: i64,
    pub native_id: String,
    pub provider: Arc<dyn Provider>,
}

struct Session {
    target: Target,
    epoch: u64,
    closed: AtomicBool,
    touched: Mutex<Instant>,
    data: AsyncMutex<Page>,
    items: Mutex<Option<ItemRetention>>,
}

struct Operation {
    provider: Arc<dyn Provider>,
    epoch: u64,
    fingerprint: String,
    created: Instant,
    result: watch::Sender<Option<RadioResult>>,
}

pub struct RadioSessions {
    sessions: Mutex<HashMap<String, Arc<Session>>>,
    operations: Mutex<HashMap<String, Arc<Operation>>>,
    validate_provider: Arc<ProviderValidator>,
    stopped: AtomicBool,
}

impl Default for RadioSessions {
    fn default() -> Self {
        Self {
            sessions: Default::default(),
            operations: Default::default(),
            stopped: AtomicBool::new(false),
            validate_provider: Arc::new(|provider| {
                provider
                    .id()
                    .and_then(qcm_core::global::provider)
                    .is_some_and(|current| Arc::ptr_eq(&current, provider))
            }),
        }
    }
}

impl RadioSessions {
    fn valid(&self, session: &Session) -> bool {
        !self.stopped.load(Ordering::Acquire)
            && !session.closed.load(Ordering::Acquire)
            && session.epoch == session.target.provider.lifecycle_epoch()
            && (self.validate_provider)(&session.target.provider)
    }

    fn session(&self, id: &str, target: &Target) -> Result<Arc<Session>, RadioError> {
        let session = self
            .sessions
            .lock()
            .unwrap()
            .get(id)
            .cloned()
            .ok_or(RadioError::Expired)?;
        if !self.valid(&session)
            || session.target.queue_id != target.queue_id
            || !Arc::ptr_eq(&session.target.provider, &target.provider)
        {
            return Err(RadioError::Expired);
        }
        *session.touched.lock().unwrap() = Instant::now();
        Ok(session)
    }

    async fn operation<F, Fut>(
        self: &Arc<Self>,
        key: String,
        fingerprint: String,
        provider: Arc<dyn Provider>,
        run: F,
    ) -> RadioResult
    where
        F: FnOnce() -> Fut + Send + 'static,
        Fut: std::future::Future<Output = RadioResult> + Send + 'static,
    {
        if self.stopped.load(Ordering::Acquire) || !(self.validate_provider)(&provider) {
            return Err(RadioError::Expired);
        }
        let (operation, fresh) = {
            let mut operations = self.operations.lock().unwrap();
            operations.retain(|_, op| {
                op.created.elapsed() < OPERATION_TTL || op.result.borrow().is_none()
            });
            if let Some(op) = operations.get(&key) {
                if !Arc::ptr_eq(&op.provider, &provider) || op.epoch != provider.lifecycle_epoch() {
                    return Err(RadioError::Expired);
                }
                if op.fingerprint != fingerprint {
                    return Err(RadioError::Conflict);
                }
                (op.clone(), false)
            } else {
                if operations.len() >= MAX_OPERATIONS {
                    return Err(RadioError::Invalid(
                        "Too many pending radio operations".into(),
                    ));
                }
                let (result, _) = watch::channel(None);
                let op = Arc::new(Operation {
                    epoch: provider.lifecycle_epoch(),
                    provider,
                    fingerprint,
                    created: Instant::now(),
                    result,
                });
                operations.insert(key, op.clone());
                (op, true)
            }
        };
        let mut receiver = operation.result.subscribe();
        if fresh {
            let operation = operation.clone();
            tokio::spawn(async move {
                use futures::FutureExt;
                let result = match tokio::time::timeout(
                    Duration::from_secs(180),
                    std::panic::AssertUnwindSafe(run()).catch_unwind(),
                )
                .await
                {
                    Ok(Ok(result)) => result,
                    _ => Err(RadioError::Invalid(
                        "Radio operation did not complete".into(),
                    )),
                };
                operation.result.send_replace(Some(result));
            });
        }
        loop {
            if let Some(result) = receiver.borrow().clone() {
                if operation.epoch != operation.provider.lifecycle_epoch()
                    || !(self.validate_provider)(&operation.provider)
                {
                    return Err(RadioError::Expired);
                }
                return result;
            }
            receiver.changed().await.map_err(|_| RadioError::Expired)?;
        }
    }

    pub async fn open(
        self: &Arc<Self>,
        ctx: Arc<Context>,
        target: Target,
        operation_id: String,
        previous_id: String,
        mut request: QueueOpenRequest,
    ) -> RadioResult {
        check_operation_id(&operation_id)?;
        request.previous_state = None;
        let configuration: BTreeMap<_, _> = request.configuration.iter().collect();
        let fingerprint = serde_json::to_string(&(
            target.queue_id,
            target.provider.id(),
            &previous_id,
            &request.reason,
            &request.input,
            configuration,
        ))
        .map_err(|e| RadioError::Invalid(e.to_string()))?;
        let key = format!("open/{operation_id}");
        let owner = self.clone();
        let page = self
            .operation(
                key,
                fingerprint,
                target.provider.clone(),
                move || async move {
                    if owner.stopped.load(Ordering::Acquire)
                        || !(owner.validate_provider)(&target.provider)
                    {
                        return Err(RadioError::Expired);
                    }
                    let epoch = target.provider.lifecycle_epoch();
                    if !previous_id.is_empty() {
                        let previous = owner.session(&previous_id, &target)?;
                        let data = previous.data.lock().await;
                        if !owner.valid(&previous) {
                            return Err(RadioError::Expired);
                        }
                        request.previous_state = Some(data.batch.state.clone());
                    }
                    let definition = target
                        .provider
                        .queue_definition(&ctx, &target.native_id)
                        .await?;
                    for field in definition.configuration {
                        request
                            .configuration
                            .entry(field.key)
                            .or_insert(field.value);
                    }
                    let batch = target
                        .provider
                        .open_queue(&ctx, &target.native_id, request)
                        .await?;
                    let page = Page {
                        session_id: uuid::Uuid::new_v4().to_string(),
                        version: 1,
                        batch,
                    };
                    let session = Arc::new(Session {
                        target,
                        epoch,
                        closed: AtomicBool::new(false),
                        touched: Mutex::new(Instant::now()),
                        data: AsyncMutex::new(page.clone()),
                        items: Mutex::new(None),
                    });
                    let validation = async {
                        validate_batch(&page.batch, true)?;
                        let items = retain_batch(&ctx, &session.target, &page.batch, vec![]).await?;
                        *session.items.lock().unwrap() = Some(items);
                        Ok::<_, RadioError>(())
                    }
                    .await;
                    let inserted = {
                        let mut sessions = owner.sessions.lock().unwrap();
                        if validation.is_ok()
                            && owner.valid(&session)
                            && sessions.len() < MAX_SESSIONS
                        {
                            sessions.insert(page.session_id.clone(), session.clone());
                            true
                        } else {
                            false
                        }
                    };
                    if !inserted {
                        let valid = owner.valid(&session);
                        owner.release(&ctx, &session).await;
                        validation?;
                        if !valid {
                            return Err(RadioError::Expired);
                        }
                        return Err(RadioError::Invalid("Too many radio sessions".into()));
                    }
                    Ok(page)
                },
            )
            .await?;
        let sessions = self.sessions.lock().unwrap();
        if sessions
            .get(&page.session_id)
            .is_none_or(|s| !self.valid(s))
        {
            return Err(RadioError::Expired);
        }
        Ok(page)
    }

    pub async fn next(
        self: &Arc<Self>,
        ctx: Arc<Context>,
        target: Target,
        session_id: String,
        operation_id: String,
        expected_version: u64,
        current_native_ids: Vec<String>,
    ) -> RadioResult {
        check_operation_id(&operation_id)?;
        let session = self.session(&session_id, &target)?;
        let fingerprint = serde_json::to_string(&(expected_version, &current_native_ids))
            .map_err(|e| RadioError::Invalid(e.to_string()))?;
        let owner = self.clone();
        let check_target = target.clone();
        let result = self
            .operation(
                format!("next/{session_id}/{operation_id}"),
                fingerprint,
                target.provider.clone(),
                move || async move {
                    let mut page = session.data.lock().await;
                    if !owner.valid(&session) {
                        return Err(RadioError::Expired);
                    }
                    if page.version != expected_version {
                        return Err(RadioError::Conflict);
                    }
                    if page.batch.exhausted {
                        return Err(RadioError::Invalid("Radio has ended".into()));
                    }
                    let retained = session.items.lock().unwrap().as_ref().map(|items| {
                        items.roots().iter()
                            .filter(|item| {
                                item.r#type == qcm_core::model::type_enum::ItemType::Song
                                    && current_native_ids.contains(&item.native_id)
                            })
                            .map(|item| item.id)
                            .collect()
                    }).unwrap_or_default();
                    let batch = target
                        .provider
                        .next_queue(
                            &ctx,
                            &target.native_id,
                            QueueNextRequest {
                                state: page.batch.state.clone(),
                                current_native_ids,
                            },
                        )
                        .await?;
                    if !owner.valid(&session) {
                        page.batch.state = batch.state;
                        return Err(RadioError::Expired);
                    }
                    validate_batch(&batch, false)?;
                    if batch.configuration != page.batch.configuration {
                        return Err(RadioError::Invalid(
                            "Radio configuration changed during continuation".into(),
                        ));
                    }
                    let items = retain_batch(&ctx, &target, &batch, retained).await?;
                    if !owner.valid(&session) {
                        page.batch.state = batch.state;
                        return Err(RadioError::Expired);
                    }
                    *session.items.lock().unwrap() = Some(items);
                    page.version += 1;
                    page.batch = batch;
                    Ok(page.clone())
                },
            )
            .await?;
        self.session(&session_id, &check_target)?;
        Ok(result)
    }

    async fn release(&self, ctx: &Context, session: &Session) {
        if !session.closed.swap(true, Ordering::AcqRel) {
            let data = session.data.lock().await;
            match tokio::time::timeout(
                Duration::from_secs(10),
                session.target.provider.close_queue(
                    ctx,
                    &session.target.native_id,
                    &data.batch.state,
                ),
            )
            .await
            {
                Ok(Ok(())) => (),
                result => log::warn!("Radio cleanup failed: {result:?}"),
            }
            session.items.lock().unwrap().take();
        }
    }

    pub async fn close(&self, ctx: &Context, id: &str) {
        let session = self.sessions.lock().unwrap().remove(id);
        if let Some(session) = session {
            self.release(ctx, &session).await;
        }
    }

    pub async fn collect(&self, ctx: &Context) {
        let expired = {
            let mut sessions = self.sessions.lock().unwrap();
            let ids: Vec<_> = sessions
                .iter()
                .filter(|(_, s)| {
                    !self.valid(s) || s.touched.lock().unwrap().elapsed() >= SESSION_TTL
                })
                .map(|(id, _)| id.clone())
                .collect();
            ids.into_iter()
                .filter_map(|id| sessions.remove(&id))
                .collect::<Vec<_>>()
        };
        for session in expired {
            self.release(ctx, &session).await;
        }
        self.operations
            .lock()
            .unwrap()
            .retain(|_, op| op.created.elapsed() < OPERATION_TTL || op.result.borrow().is_none());
    }

    pub async fn invalidate(&self, ctx: &Context, provider_id: i64) {
        let removed = {
            let mut sessions = self.sessions.lock().unwrap();
            let ids: Vec<_> = sessions
                .iter()
                .filter(|(_, s)| s.target.provider.id() == Some(provider_id))
                .map(|(id, _)| id.clone())
                .collect();
            ids.into_iter()
                .filter_map(|id| sessions.remove(&id))
                .collect::<Vec<_>>()
        };
        for session in removed {
            self.release(ctx, &session).await;
        }
    }

    pub async fn shutdown(&self, ctx: &Context) {
        self.stopped.store(true, Ordering::Release);
        self.collect(ctx).await;
    }
}

async fn retain_batch(
    ctx: &Context,
    target: &Target,
    batch: &QueueBatch,
    mut ids: Vec<i64>,
) -> Result<ItemRetention, RadioError> {
    ids.extend(batch.songs.iter().map(|song| song.id));
    ids.extend(batch.presentation.source_item_id);
    let provider = target.provider.id().ok_or(RadioError::Expired)?;
    ctx.sync_coordinator.retain_items(&ctx.db, provider, ids).await
        .map_err(ProviderError::from)
        .map_err(RadioError::from)
}

fn check_operation_id(id: &str) -> Result<(), RadioError> {
    let (timestamp, uuid) = id
        .split_once(':')
        .ok_or_else(|| RadioError::Invalid("Invalid radio operation ID".into()))?;
    uuid::Uuid::parse_str(uuid)
        .map_err(|_| RadioError::Invalid("Invalid radio operation ID".into()))?;
    let timestamp = timestamp
        .parse::<u128>()
        .map_err(|_| RadioError::Invalid("Invalid radio operation timestamp".into()))?;
    let now = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_millis();
    if timestamp > now + 30_000 || now.saturating_sub(timestamp) >= OPERATION_TTL.as_millis() {
        return Err(RadioError::Expired);
    }
    Ok(())
}

fn validate_batch(batch: &QueueBatch, opening: bool) -> Result<(), RadioError> {
    if batch.state.len() > MAX_STATE_BYTES {
        return Err(RadioError::Invalid("Radio state exceeds 64 KiB".into()));
    }
    if batch.songs.iter().any(|s| s.id <= 0) {
        return Err(RadioError::Invalid(
            "Radio returned unsynchronized songs".into(),
        ));
    }
    if opening && !batch.songs.iter().any(|song| song.can_play) {
        return Err(RadioError::Invalid(
            "Radio returned no playable songs".into(),
        ));
    }
    Ok(())
}

#[cfg(test)]
mod tests;
