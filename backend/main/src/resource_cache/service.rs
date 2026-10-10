use super::{
    format::{identity, Geometry},
    policy::{self, Policy},
    store::{Metadata, Store, Version},
};
use crate::{
    error::ProcessError,
    http::{body_type::ResponseBody, error::HttpError},
};
use anyhow::{ensure, Result};
use bytes::{Bytes, BytesMut};
use futures_util::{StreamExt, TryStreamExt};
use http_body_util::{BodyExt, StreamBody};
use hyper::{body::Frame, Response, StatusCode};
use qcm_core::resource::PreparedResource;
use reqwest::header::HeaderMap;
use std::{
    collections::HashMap,
    ops::Range,
    path::PathBuf,
    sync::{
        atomic::{AtomicU64, Ordering},
        Arc, Weak,
    },
    time::Duration,
};
use tokio::sync::{watch, Mutex, Notify, OwnedSemaphorePermit, Semaphore};

const FETCH_BYTES: u64 = 4 * 1024 * 1024;
const OUTPUT_BYTES: usize = 64 * 1024;
const MAX_READERS: usize = 16;
type DownloadKey = (i64, u64, u64);

#[derive(Clone)]
pub struct ResourceRequest {
    pub provider: i64,
    pub kind: i32,
    pub scope: String,
    pub prepared: Arc<PreparedResource>,
    pub headers: HeaderMap,
}
pub struct ResourceCache {
    pub(super) store: Option<Store>,
    opens: Mutex<HashMap<String, Weak<Mutex<()>>>>,
    downloads: Mutex<HashMap<DownloadKey, Weak<Download>>>,
    readers: Arc<Semaphore>,
    buffers: Arc<Semaphore>,
    requests: Arc<Semaphore>,
    per_resource: Mutex<HashMap<i64, Weak<Semaphore>>>,
    stop: watch::Sender<bool>,
    tasks: Mutex<tokio::task::JoinSet<()>>,
}
#[derive(Clone)]
enum Progress {
    Running,
    Complete,
    Failed(DownloadFailure),
}
#[derive(Clone, Debug, thiserror::Error)]
enum DownloadFailure {
    #[error("cached representation changed")]
    Changed,
    #[error("upstream resource status {0}")]
    Upstream(StatusCode, HeaderMap),
    #[error("{0}")]
    Other(String),
}
struct Download {
    provider: i64,
    cancel: watch::Sender<bool>,
    range: Range<u64>,
    blocks: Mutex<HashMap<u64, Bytes>>,
    progress: watch::Sender<Progress>,
    readers: Mutex<Vec<Weak<AtomicU64>>>,
    readable: Notify,
}
struct Reader {
    cache: Arc<ResourceCache>,
    version: Version,
    prepared: Arc<PreparedResource>,
    base: HeaderMap,
    cursor: u64,
    end: u64,
    block: Option<(u64, Bytes)>,
    initial: Option<Arc<Download>>,
    _permit: OwnedSemaphorePermit,
    position: Arc<AtomicU64>,
    only_cached: bool,
    window: u64,
    sequential: u32,
}
impl Drop for Reader {
    fn drop(&mut self) {
        if let Some(download) = &self.initial {
            download.readable.notify_one();
        }
    }
}
impl ResourceCache {
    pub async fn open(root: PathBuf) -> Result<Arc<Self>> {
        let (stop, _) = watch::channel(false);
        let store = match Store::open(root.join("resources-v1")).await {
            Ok(store) => Some(store),
            Err(err) => {
                log::warn!("resource cache unavailable: {err}");
                None
            }
        };
        Ok(Arc::new(Self {
            store,
            opens: Mutex::new(HashMap::new()),
            downloads: Mutex::new(HashMap::new()),
            readers: Arc::new(Semaphore::new(MAX_READERS)),
            buffers: Arc::new(Semaphore::new(8)),
            requests: Arc::new(Semaphore::new(4)),
            per_resource: Mutex::new(HashMap::new()),
            stop,
            tasks: Mutex::new(tokio::task::JoinSet::new()),
        }))
    }
    fn storage(&self) -> &Store {
        self.store.as_ref().expect("persistent resource operation")
    }
    pub async fn usage(&self) -> Result<super::store::Usage> {
        if self.store.is_none() {
            return Ok(Default::default());
        }
        self.storage().usage().await
    }
    pub async fn invalidate(&self, provider: i64) -> Result<()> {
        if self.store.is_none() {
            return Ok(());
        }
        for download in self
            .downloads
            .lock()
            .await
            .values()
            .filter_map(Weak::upgrade)
            .filter(|d| d.provider == provider)
        {
            download.cancel.send_replace(true);
            download
                .progress
                .send_replace(Progress::Failed(DownloadFailure::Other(
                    "provider invalidated".into(),
                )));
        }
        self.storage().invalidate(provider).await
    }
    pub async fn shutdown(&self) -> Result<()> {
        if self.stop.send_replace(true) {
            return Ok(());
        }
        self.readers.close();
        self.buffers.close();
        self.requests.close();
        let mut tasks = self.tasks.lock().await;
        if tokio::time::timeout(Duration::from_secs(5), async {
            while tasks.join_next().await.is_some() {}
        })
        .await
        .is_err()
        {
            tasks.abort_all();
            while tasks.join_next().await.is_some() {}
        }
        if self.store.is_some() {
            self.storage().shutdown().await
        } else {
            Ok(())
        }
    }
    pub async fn response(
        self: &Arc<Self>,
        request: ResourceRequest,
    ) -> Result<Response<ResponseBody>, ProcessError> {
        let mut result = self.respond(request.clone()).await;
        if result.as_ref().is_err_and(|error| {
            matches!(
                error.downcast_ref::<DownloadFailure>(),
                Some(DownloadFailure::Changed)
            )
        }) {
            result = self.respond(request).await;
        }
        match result {
            Ok(response) => Ok(response),
            Err(error) => {
                if let Some(DownloadFailure::Upstream(status, headers)) =
                    error.downcast_ref::<DownloadFailure>()
                {
                    let mut headers = headers.clone();
                    headers.remove("content-encoding");
                    empty(*status, headers).map_err(ProcessError::Resource)
                } else {
                    Err(ProcessError::Resource(error))
                }
            }
        }
    }
    pub async fn forward(
        &self,
        response: reqwest::Response,
    ) -> Result<Response<ResponseBody>, ProcessError> {
        let permit = self
            .readers
            .clone()
            .acquire_owned()
            .await
            .map_err(|e| anyhow::anyhow!("{e}"))?;
        passthrough(response, permit).map_err(ProcessError::Internal)
    }
    async fn respond(self: &Arc<Self>, request: ResourceRequest) -> Result<Response<ResponseBody>> {
        if *self.stop.borrow() {
            return Err(anyhow::anyhow!("resource service stopped").into());
        }
        let permit = self
            .readers
            .clone()
            .acquire_owned()
            .await
            .map_err(|e| anyhow::anyhow!("{e}"))?;
        let prepared = request.prepared;
        ensure!(prepared.is_valid(), "provider invalidated");
        let base = request_headers(prepared.headers(), &request.headers);
        if self.store.is_none() {
            if policy::only_cached(&request.headers) {
                return empty(StatusCode::GATEWAY_TIMEOUT, HeaderMap::new());
            }
            return passthrough(
                self.fetch(&prepared, downstream_request(&base, &request.headers))
                    .await?,
                permit,
            );
        }
        let key = identity(&serde_json::to_vec(&(
            request.provider,
            &request.scope,
            prepared.url().as_str(),
            canonical_headers(&base),
        ))?)?;
        let open = {
            let mut opens = self.opens.lock().await;
            opens.retain(|_, lock| lock.strong_count() > 0);
            if let Some(lock) = opens.get(&key).and_then(Weak::upgrade) {
                lock
            } else {
                let lock = Arc::new(Mutex::new(()));
                opens.insert(key.clone(), Arc::downgrade(&lock));
                lock
            }
        };
        let _opened = open.lock().await;
        let bypass = policy::request_no_store(&request.headers);
        let mut cached = if bypass {
            None
        } else {
            self.storage().lookup(key.clone()).await?
        };
        if let Some(old) = &cached {
            let downloading = self
                .downloads
                .lock()
                .await
                .iter()
                .any(|((id, _, _), d)| *id == old.id && d.strong_count() > 0);
            if !downloading
                && (old.meta.policy.strong().is_none() || !old.meta.ranges)
                && !self.storage().complete(old.clone()).await?
            {
                cached = None;
            }
        }
        if policy::only_cached(&request.headers)
            && !cached
                .as_ref()
                .is_some_and(|v| v.meta.policy.fresh(&request.headers))
        {
            return empty(StatusCode::GATEWAY_TIMEOUT, HeaderMap::new());
        }
        if bypass {
            return passthrough(
                self.fetch(&prepared, downstream_request(&base, &request.headers))
                    .await?,
                permit,
            );
        }
        let mut initial = None;
        let mut response_range = 0..0;
        let version = if let Some(old) = cached
            .as_ref()
            .filter(|v| v.meta.policy.fresh(&request.headers))
        {
            old.clone()
        } else {
            let mut upstream_headers = base.clone();
            if let Some(old) = &cached {
                upstream_headers.extend(old.meta.policy.conditional());
            }
            if request.kind == 1 {
                let start = if request.headers.contains_key("if-range") {
                    0
                } else {
                    policy::value(&request.headers, "range")
                        .and_then(|v| v.strip_prefix("bytes="))
                        .and_then(|v| v.split_once('-'))
                        .and_then(|(s, _)| s.parse::<u64>().ok())
                        .unwrap_or(0)
                };
                let start = start / super::format::MAX_BLOCK * super::format::MAX_BLOCK;
                let length = if start == 0 {
                    FETCH_BYTES
                } else {
                    super::format::MAX_BLOCK
                };
                upstream_headers.insert(
                    "range",
                    format!("bytes={start}-{}", start.saturating_add(length - 1))
                        .parse()
                        .unwrap(),
                );
            }
            let sent = policy::now();
            let mut response = self.fetch(&prepared, upstream_headers).await?;
            let received = policy::now();
            if response.status() == StatusCode::NOT_MODIFIED {
                let Some(old) = cached else {
                    return Err(anyhow::anyhow!("unexpected upstream 304"));
                };
                ensure_validation(&old, &response)?;
                let headers = policy::merge_validation(&old.meta.headers, response.headers());
                let mut meta = old.meta.clone();
                meta.policy = Policy::from_response(&headers, sent, received);
                meta.headers = policy::saved(&headers);
                if !meta.policy.store {
                    self.storage().retire(old.clone()).await?;
                    drop(response);
                    return passthrough(
                        self.fetch(&prepared, downstream_request(&base, &request.headers))
                            .await?,
                        permit,
                    );
                }
                self.storage().update(old.clone(), meta.clone()).await?;
                Version { meta, ..old }
            } else {
                if !matches!(
                    response.status(),
                    StatusCode::OK | StatusCode::PARTIAL_CONTENT
                ) {
                    return passthrough(response, permit);
                }
                let total = response_geometry(&response)?;
                let policy = Policy::from_response(response.headers(), sent, received);
                if !policy.store || total.is_none() {
                    if response.status() == StatusCode::PARTIAL_CONTENT {
                        drop(response);
                        response = self
                            .fetch(&prepared, downstream_request(&base, &request.headers))
                            .await?;
                    }
                    return passthrough(response, permit);
                }
                let (length, range) = total.unwrap();
                if response.status() == StatusCode::PARTIAL_CONTENT && policy.strong().is_none() {
                    drop(response);
                    response = self.fetch(&prepared, base.clone()).await?;
                    ensure!(
                        response.status() == StatusCode::OK,
                        "unvalidated partial resource"
                    );
                    if !Policy::from_response(response.headers(), sent, policy::now()).store {
                        return passthrough(response, permit);
                    }
                    let Some((length, range)) = response_geometry(&response)? else {
                        return passthrough(response, permit);
                    };
                    response_range = range;
                    let meta = metadata(
                        &request.scope,
                        request.provider,
                        request.kind,
                        &key,
                        length,
                        &response,
                        sent,
                        policy::now(),
                    )?;
                    let version = match self.storage().create(meta).await {
                        Ok(version) => version,
                        Err(err) => {
                            log::warn!("resource cache unavailable: {err}");
                            return passthrough(response, permit);
                        }
                    };
                    initial = Some(response);
                    version
                } else {
                    response_range = range;
                    let meta = metadata(
                        &request.scope,
                        request.provider,
                        request.kind,
                        &key,
                        length,
                        &response,
                        sent,
                        received,
                    )?;
                    let version = match self.storage().create(meta).await {
                        Ok(version) => version,
                        Err(err) => {
                            log::warn!("resource cache unavailable: {err}");
                            drop(response);
                            return passthrough(
                                self.fetch(&prepared, downstream_request(&base, &request.headers))
                                    .await?,
                                permit,
                            );
                        }
                    };
                    initial = Some(response);
                    version
                }
            }
        };
        if policy::not_modified(&version.meta.policy, &request.headers) {
            return empty(
                StatusCode::NOT_MODIFIED,
                policy::restore(&version.meta.headers),
            );
        }
        let range = match client_range(&request.headers, &version.meta)? {
            Some(range) => range,
            None => {
                let mut headers = policy::restore(&version.meta.headers);
                headers.insert(
                    "content-range",
                    format!("bytes */{}", version.meta.geometry.length)
                        .parse()
                        .unwrap(),
                );
                return empty(StatusCode::RANGE_NOT_SATISFIABLE, headers);
            }
        };
        let position = Arc::new(AtomicU64::new(range.start));
        let only_cached = policy::only_cached(&request.headers);
        if only_cached
            && !self
                .storage()
                .covered(version.clone(), range.clone())
                .await?
        {
            return empty(StatusCode::GATEWAY_TIMEOUT, HeaderMap::new());
        }
        let initial = if let Some(response) = initial {
            if range.start >= response_range.start && range.start < response_range.end {
                Some(
                    self.download(
                        version.clone(),
                        prepared.clone(),
                        base.clone(),
                        response_range,
                        Some(response),
                        position.clone(),
                    )
                    .await?,
                )
            } else {
                None
            }
        } else {
            let existing = self
                .downloads
                .lock()
                .await
                .iter()
                .filter(|((id, _, _), _)| *id == version.id)
                .filter_map(|(_, d)| d.upgrade())
                .find(|d| d.range.contains(&range.start));
            if let Some(download) = &existing {
                download
                    .readers
                    .lock()
                    .await
                    .push(Arc::downgrade(&position));
            }
            existing
        };
        let mut headers = policy::restore(&version.meta.headers);
        headers.insert(
            "age",
            version.meta.policy.age().to_string().parse().unwrap(),
        );
        headers.insert(
            "content-length",
            (range.end - range.start).to_string().parse().unwrap(),
        );
        if version.meta.ranges || self.storage().complete(version.clone()).await? {
            headers.insert("accept-ranges", "bytes".parse().unwrap());
        }
        let partial = range.start != 0
            || range.end != version.meta.geometry.length
            || range_requested(&request.headers, &version.meta);
        if partial {
            headers.insert(
                "content-range",
                format!(
                    "bytes {}-{}/{}",
                    range.start,
                    range.end - 1,
                    version.meta.geometry.length
                )
                .parse()
                .unwrap(),
            );
        }
        let (first, initial) = if range.start < range.end && !only_cached {
            let (bytes, download) = self
                .block(
                    version.clone(),
                    prepared.clone(),
                    base.clone(),
                    range.start / version.meta.geometry.block,
                    initial,
                    position.clone(),
                    if range.start == 0 {
                        FETCH_BYTES
                    } else {
                        super::format::MAX_BLOCK
                    },
                )
                .await?;
            (
                Some((range.start / version.meta.geometry.block, bytes)),
                download,
            )
        } else {
            (None, initial)
        };
        ensure!(
            version.is_valid() && prepared.is_valid(),
            "provider invalidated"
        );
        let reader = Reader {
            cache: self.clone(),
            version,
            prepared,
            base,
            cursor: range.start,
            end: range.end,
            block: first,
            initial,
            _permit: permit,
            position,
            only_cached,
            window: if range.start == 0 {
                FETCH_BYTES
            } else {
                super::format::MAX_BLOCK
            },
            sequential: 0,
        };
        let body = futures_util::stream::try_unfold(reader, |mut reader| async move {
            if *reader.cache.stop.borrow()
                || !reader.prepared.is_valid()
                || !reader.version.is_valid()
            {
                return Err(HttpError::Cache("resource reader closed".into()));
            }
            if reader.cursor == reader.end {
                return Ok(None);
            }
            let index = reader.cursor / reader.version.meta.geometry.block;
            if reader
                .initial
                .as_ref()
                .is_some_and(|d| !d.range.contains(&reader.cursor))
            {
                reader.initial = None;
            }
            if reader.block.as_ref().is_none_or(|(i, _)| *i != index) {
                if reader.block.as_ref().is_some_and(|(i, _)| *i + 1 == index) {
                    reader.sequential += 1;
                    if reader.sequential >= 4 {
                        reader.window = (reader.window * 2).min(FETCH_BYTES);
                        reader.sequential = 0;
                    }
                }
                let (bytes, download) = if reader.only_cached {
                    (
                        reader
                            .cache
                            .storage()
                            .read(reader.version.clone(), index)
                            .await
                            .map_err(|e| HttpError::Cache(e.to_string()))?
                            .ok_or_else(|| HttpError::Cache("cached block unavailable".into()))?,
                        None,
                    )
                } else {
                    reader
                        .cache
                        .block(
                            reader.version.clone(),
                            reader.prepared.clone(),
                            reader.base.clone(),
                            index,
                            reader.initial.clone(),
                            reader.position.clone(),
                            reader.window,
                        )
                        .await
                        .map_err(|e| HttpError::Cache(e.to_string()))?
                };
                reader.initial = download;
                reader.block = Some((index, bytes));
            }
            let bytes = &reader.block.as_ref().unwrap().1;
            let offset = (reader.cursor % reader.version.meta.geometry.block) as usize;
            let length = OUTPUT_BYTES
                .min(bytes.len() - offset)
                .min((reader.end - reader.cursor) as usize);
            let frame = Frame::data(bytes.slice(offset..offset + length));
            reader.cursor += length as u64;
            reader.position.store(reader.cursor, Ordering::Release);
            if let Some(download) = &reader.initial {
                download.readable.notify_one();
            }
            Ok::<_, HttpError>(Some((frame, reader)))
        });
        let mut response = Response::new(ResponseBody::Resource(BodyExt::boxed_unsync(
            StreamBody::new(body),
        )));
        *response.headers_mut() = headers;
        *response.status_mut() = if partial {
            StatusCode::PARTIAL_CONTENT
        } else {
            StatusCode::OK
        };
        Ok(response)
    }
    async fn block(
        self: &Arc<Self>,
        version: Version,
        prepared: Arc<PreparedResource>,
        base: HeaderMap,
        index: u64,
        initial: Option<Arc<Download>>,
        position: Arc<AtomicU64>,
        window: u64,
    ) -> Result<(Bytes, Option<Arc<Download>>)> {
        if let Some(bytes) = self.storage().read(version.clone(), index).await? {
            return Ok((bytes, initial));
        }
        let download = if let Some(initial) = initial {
            initial
        } else {
            if version.meta.policy.strong().is_none() {
                self.storage().retire(version.clone()).await?;
                return Err(DownloadFailure::Changed.into());
            }
            let block = version.meta.geometry.block;
            let start = index * block;
            let range = if version.meta.ranges && version.meta.policy.strong().is_some() {
                start..(start + window).min(version.meta.geometry.length)
            } else {
                0..version.meta.geometry.length
            };
            self.download(version.clone(), prepared, base, range, None, position)
                .await?
        };
        let mut progress = download.progress.subscribe();
        let mut stop = self.stop.subscribe();
        loop {
            ensure!(!*stop.borrow(), "resource service stopped");
            let state = progress.borrow_and_update().clone();
            if let Some(bytes) = download.blocks.lock().await.get(&index).cloned() {
                return Ok((bytes, Some(download.clone())));
            }
            if let Some(bytes) = self.storage().read(version.clone(), index).await? {
                return Ok((bytes, Some(download.clone())));
            }
            match state {
                Progress::Failed(error) => return Err(error.into()),
                Progress::Complete => anyhow::bail!("download did not produce requested block"),
                Progress::Running => (),
            }
            tokio::select! { result = progress.changed() => { result?; }, _ = stop.changed() => anyhow::bail!("resource service stopped") }
        }
    }
    async fn download(
        self: &Arc<Self>,
        version: Version,
        prepared: Arc<PreparedResource>,
        base: HeaderMap,
        mut range: Range<u64>,
        response: Option<reqwest::Response>,
        position: Arc<AtomicU64>,
    ) -> Result<Arc<Download>> {
        let mut downloads = self.downloads.lock().await;
        downloads.retain(|_, v| v.strong_count() > 0);
        if let Some(value) = downloads
            .iter()
            .filter(|((id, _, _), _)| *id == version.id)
            .filter_map(|(_, d)| d.upgrade())
            .find(|d| d.range.contains(&position.load(Ordering::Acquire)))
        {
            value.readers.lock().await.push(Arc::downgrade(&position));
            return Ok(value);
        }
        if response.is_none() && version.meta.ranges && version.meta.policy.strong().is_some() {
            range = self.storage().missing_range(version.clone(), range).await?;
            if let Some(end) = downloads
                .iter()
                .filter(|((id, _, _), _)| *id == version.id)
                .filter_map(|(_, d)| d.upgrade())
                .map(|d| d.range.start)
                .filter(|start| *start > range.start && *start < range.end)
                .min()
            {
                range.end = end;
            }
        }
        let key = (version.id, range.start, range.end);
        let (progress, _) = watch::channel(Progress::Running);
        let (cancel, _) = watch::channel(false);
        let download = Arc::new(Download {
            provider: version.meta.provider,
            cancel,
            range: range.clone(),
            blocks: Mutex::new(HashMap::new()),
            progress,
            readers: Mutex::new(vec![Arc::downgrade(&position)]),
            readable: Notify::new(),
        });
        downloads.insert(key, Arc::downgrade(&download));
        drop(downloads);
        let cache = self.clone();
        let target = download.clone();
        let mut tasks = self.tasks.lock().await;
        if *self.stop.borrow() {
            target
                .progress
                .send_replace(Progress::Failed(DownloadFailure::Other(
                    "resource service stopped".into(),
                )));
            anyhow::bail!("resource service stopped");
        }
        while tasks.try_join_next().is_some() {}
        tasks.spawn(async move {
            let result = cache
                .fetch_blocks(&target, version, prepared, base, range, response)
                .await;
            target.progress.send_replace(match result {
                Ok(()) => Progress::Complete,
                Err(err) => Progress::Failed(
                    err.downcast::<DownloadFailure>()
                        .unwrap_or_else(|err| DownloadFailure::Other(err.to_string())),
                ),
            });
            cache.downloads.lock().await.remove(&key);
        });
        Ok(download)
    }
    async fn fetch_blocks(
        self: &Arc<Self>,
        download: &Arc<Download>,
        version: Version,
        prepared: Arc<PreparedResource>,
        mut headers: HeaderMap,
        range: Range<u64>,
        response: Option<reqwest::Response>,
    ) -> Result<()> {
        let resource_limit = {
            let mut limits = self.per_resource.lock().await;
            limits.retain(|_, s| s.strong_count() > 0);
            if let Some(limit) = limits.get(&version.id).and_then(Weak::upgrade) {
                limit
            } else {
                let limit = Arc::new(Semaphore::new(2));
                limits.insert(version.id, Arc::downgrade(&limit));
                limit
            }
        };
        let _resource_request = resource_limit.acquire_owned().await?;
        let _buffer = self.buffers.clone().acquire_owned().await?;
        let _request = self.requests.clone().acquire_owned().await?;
        let mut cancel = download.cancel.subscribe();
        ensure!(!*cancel.borrow(), "provider invalidated");
        let mut skip = 0u64;
        let mut clipped = false;
        let response = if let Some(response) = response {
            response
        } else {
            if version.meta.ranges && version.meta.policy.strong().is_some() {
                headers.insert(
                    "range",
                    format!("bytes={}-{}", range.start, range.end - 1).parse()?,
                );
                headers.insert("if-range", version.meta.policy.strong().unwrap().parse()?);
            }
            let response = tokio::select! {
                result=self.execute(&prepared,headers)=>result?,
                _=cancel.changed()=>anyhow::bail!("provider invalidated"),
                _=self.wait_stopped()=>anyhow::bail!("resource service stopped"),
            };
            if !matches!(
                response.status(),
                StatusCode::OK | StatusCode::PARTIAL_CONTENT
            ) {
                let _ = self.storage().retire(version.clone()).await;
                return Err(DownloadFailure::Upstream(
                    response.status(),
                    policy::forwarded(response.headers()),
                )
                .into());
            }
            let (length, actual) = response_geometry(&response)?
                .ok_or_else(|| anyhow::anyhow!("resource length disappeared"))?;
            if length != version.meta.geometry.length
                || identity(response.url().as_str().as_bytes())? != version.meta.target
            {
                self.storage().retire(version.clone()).await?;
                return Err(DownloadFailure::Changed.into());
            }
            if let Some(expected) = version.meta.policy.strong() {
                if policy::value(response.headers(), "etag") != Some(expected) {
                    self.storage().retire(version.clone()).await?;
                    return Err(DownloadFailure::Changed.into());
                }
            } else {
                ensure!(
                    response.status() == StatusCode::OK,
                    "cannot combine unvalidated partial responses"
                );
            }
            if response.status() == StatusCode::PARTIAL_CONTENT {
                ensure!(actual == range, "upstream returned wrong range");
            } else {
                if range != actual {
                    ensure!(
                        version.meta.policy.strong().is_some(),
                        "cannot resume unvalidated representation"
                    );
                    skip = range.start;
                    clipped = true;
                }
            }
            response
        };
        let cacheable =
            Policy::from_response(response.headers(), policy::now(), policy::now()).store;
        if !cacheable {
            self.storage().retire(version.clone()).await?;
        }
        let mut body = response.bytes_stream();
        let mut buffered = BytesMut::new();
        let mut cursor = range.start;
        let mut batch = Vec::new();
        let mut batch_bytes = 0;
        let mut persist = cacheable;
        let mut stop = self.stop.subscribe();
        let mut last_commit = tokio::time::Instant::now();
        let result:Result<()>=async {
        loop {
            if clipped && cursor==range.end {break;}
            ensure!(
                version.is_valid() && prepared.is_valid(),
                "provider invalidated"
            );
            if download
                .readers
                .lock()
                .await
                .iter()
                .all(|r| r.strong_count() == 0)
            {
                break;
            }
            let chunk = tokio::select! {
                chunk = tokio::time::timeout(Duration::from_secs(30), body.next()) => chunk?,
                _ = stop.changed() => anyhow::bail!("resource service stopped"),
                _ = cancel.changed() => anyhow::bail!("provider invalidated"),
                _ = download.readable.notified() => continue,
                _ = tokio::time::sleep_until(last_commit+Duration::from_millis(250)), if !batch.is_empty() => {
                    persist=self.flush_batch(download,&version,&mut batch).await;
                    batch_bytes=0; last_commit=tokio::time::Instant::now(); continue;
                },
            };
            let Some(chunk) = chunk else {
                ensure!(
                    cursor == range.end && buffered.is_empty(),
                    "truncated resource body"
                );
                break;
            };
            let mut chunk = chunk.map_err(|err|err.without_url())?;
            if skip>0 {let taken=skip.min(chunk.len() as u64) as usize;skip-=taken as u64;chunk=chunk.slice(taken..);}
            if clipped {chunk=chunk.slice(..chunk.len().min((range.end-cursor-buffered.len() as u64) as usize));}
            ensure!(
                cursor + buffered.len() as u64 + chunk.len() as u64 <= range.end,
                "resource body exceeds declared range"
            );
            while !chunk.is_empty() {
                let index = cursor / version.meta.geometry.block;
                let block_range = version.meta.geometry.range(index)?;
                ensure!(cursor == block_range.start, "unaligned upstream range");
                let expected = (block_range.end - block_range.start) as usize;
                let take = (expected - buffered.len()).min(chunk.len());
                buffered.extend_from_slice(&chunk[..take]);
                chunk = chunk.slice(take..);
                if buffered.len() == expected {
                    let bytes = buffered.split().freeze();
                    if !persist {
                        loop {
                            let min = download
                                .readers
                                .lock()
                                .await
                                .iter()
                                .filter_map(Weak::upgrade)
                                .map(|p| p.load(Ordering::Acquire))
                                .min();
                            let Some(min) = min else {
                                return Ok(());
                            };
                            let mut blocks = download.blocks.lock().await;
                            blocks.retain(|i, _| (*i + 1) * version.meta.geometry.block > min);
                            if blocks.values().map(Bytes::len).sum::<usize>() + bytes.len()
                                <= FETCH_BYTES as usize
                            {
                                break;
                            }
                            drop(blocks);
                            tokio::select! { _=download.readable.notified()=>(), _=tokio::time::sleep(Duration::from_millis(100))=>(), _=stop.changed()=>anyhow::bail!("resource service stopped"), _=cancel.changed()=>anyhow::bail!("provider invalidated") }
                        }
                    }
                    download.blocks.lock().await.insert(index, bytes.clone());
                    download.progress.send_replace(Progress::Running);
                    cursor = block_range.end;
                    if persist {
                        batch_bytes += bytes.len();
                        batch.push((index, bytes));
                    }
                    if persist && (batch_bytes >= 1024 * 1024 || cursor == range.end) {
                        persist=self.flush_batch(download,&version,&mut batch).await;
                        batch_bytes = 0;
                        last_commit=tokio::time::Instant::now();
                    }
                }
            }
        }
        Ok(())
        }.await;
        if !batch.is_empty() {
            self.flush_batch(download, &version, &mut batch).await;
        }
        result
    }
    async fn flush_batch(
        &self,
        download: &Download,
        version: &Version,
        batch: &mut Vec<(u64, Bytes)>,
    ) -> bool {
        let indexes: Vec<_> = batch.iter().map(|(i, _)| *i).collect();
        let result = self
            .storage()
            .write(version.clone(), std::mem::take(batch))
            .await;
        if let Err(err) = result {
            log::warn!("resource cache disabled for download: {err}");
            return false;
        }
        let mut blocks = download.blocks.lock().await;
        for index in indexes {
            blocks.remove(&index);
        }
        download.progress.send_replace(Progress::Running);
        true
    }
    async fn wait_stopped(&self) {
        let mut stopped = self.stop.subscribe();
        if !*stopped.borrow() {
            let _ = stopped.changed().await;
        }
    }
    async fn fetch(
        &self,
        prepared: &PreparedResource,
        headers: HeaderMap,
    ) -> Result<reqwest::Response> {
        let _request = self.requests.clone().acquire_owned().await?;
        self.execute(prepared, headers).await
    }
    async fn execute(
        &self,
        prepared: &PreparedResource,
        headers: HeaderMap,
    ) -> Result<reqwest::Response> {
        for attempt in 0..2 {
            let result = tokio::select! { result=prepared.fetch(headers.clone())=>result, _=self.wait_stopped()=>anyhow::bail!("resource service stopped") };
            match result {
                Err(qcm_core::error::ProviderError::Request(err))
                    if attempt == 0 && (err.is_connect() || err.is_timeout()) =>
                {
                    tokio::select! {_=tokio::time::sleep(Duration::from_millis(250))=>(),_=self.wait_stopped()=>anyhow::bail!("resource service stopped")}
                }
                other => return Ok(other?),
            }
        }
        unreachable!()
    }
}

fn metadata(
    _scope: &str,
    provider: i64,
    kind: i32,
    key: &str,
    length: u64,
    response: &reqwest::Response,
    sent: u64,
    received: u64,
) -> Result<Metadata> {
    Ok(Metadata {
        key: key.into(),
        provider,
        kind,
        generation: uuid::Uuid::new_v4().simple().to_string(),
        geometry: Geometry::new(length)?,
        headers: policy::saved(response.headers()),
        policy: Policy::from_response(response.headers(), sent, received),
        target: identity(response.url().as_str().as_bytes())?,
        ranges: response.status() == StatusCode::PARTIAL_CONTENT,
    })
}
fn ensure_validation(old: &Version, response: &reqwest::Response) -> Result<()> {
    if let Some(length) = policy::value(response.headers(), "content-length") {
        ensure!(
            length.parse::<u64>().ok() == Some(old.meta.geometry.length),
            "304 representation length changed"
        );
    }
    ensure!(
        identity(response.url().as_str().as_bytes())? == old.meta.target,
        "validation target changed"
    );
    if let Some(etag) = policy::value(response.headers(), "etag") {
        ensure!(
            old.meta.policy.etag.as_deref() == Some(etag),
            "304 validator changed"
        );
    }
    Ok(())
}
fn canonical_headers(headers: &HeaderMap) -> Vec<(String, String)> {
    let mut values: Vec<_> = headers
        .iter()
        .filter(|(k, _)| {
            !matches!(
                k.as_str(),
                "cache-control"
                    | "pragma"
                    | "range"
                    | "if-range"
                    | "if-none-match"
                    | "if-modified-since"
            )
        })
        .map(|(k, v)| (k.to_string(), format!("{:?}", v.as_bytes())))
        .collect();
    values.sort();
    values
}
fn request_headers(prepared: &HeaderMap, client: &HeaderMap) -> HeaderMap {
    let mut headers = prepared.clone();
    for key in [
        "accept",
        "accept-language",
        "icy-metadata",
        "cache-control",
        "pragma",
    ] {
        if let Some(v) = client.get(key) {
            headers.insert(
                reqwest::header::HeaderName::from_bytes(key.as_bytes()).unwrap(),
                v.clone(),
            );
        }
    }
    headers.remove("range");
    headers.remove("if-range");
    headers.remove("if-none-match");
    headers.remove("if-modified-since");
    headers
}
fn downstream_request(base: &HeaderMap, client: &HeaderMap) -> HeaderMap {
    let mut headers = base.clone();
    for key in ["range", "if-range", "if-none-match", "if-modified-since"] {
        if let Some(v) = client.get(key) {
            headers.insert(
                reqwest::header::HeaderName::from_bytes(key.as_bytes()).unwrap(),
                v.clone(),
            );
        }
    }
    headers
}
fn response_geometry(response: &reqwest::Response) -> Result<Option<(u64, Range<u64>)>> {
    if response.status() == StatusCode::PARTIAL_CONTENT {
        let cr = policy::value(response.headers(), "content-range")
            .and_then(crate::http::range::parse_content_range)
            .ok_or_else(|| anyhow::anyhow!("invalid Content-Range"))?;
        ensure!(
            cr.start <= cr.end && cr.end < cr.full,
            "invalid response range"
        );
        ensure!(
            response
                .content_length()
                .is_none_or(|n| n == cr.end - cr.start + 1),
            "range length mismatch"
        );
        if response.content_length().is_none() {
            return Ok(None);
        }
        Ok(Some((cr.full, cr.start..cr.end + 1)))
    } else {
        Ok(response.content_length().map(|n| (n, 0..n)))
    }
}
fn range_requested(headers: &HeaderMap, meta: &Metadata) -> bool {
    if let Some(if_range) = policy::value(headers, "if-range") {
        if meta.policy.strong() != Some(if_range) {
            return false;
        }
    }
    let Some(text) = policy::value(headers, "range").and_then(|v| v.strip_prefix("bytes=")) else {
        return false;
    };
    if text.contains(',') {
        return false;
    }
    let Some((start, end)) = text.split_once('-') else {
        return false;
    };
    if start.is_empty() {
        return end.parse::<u64>().is_ok();
    }
    start
        .parse::<u64>()
        .is_ok_and(|start| end.is_empty() || end.parse::<u64>().is_ok_and(|end| end >= start))
}
fn client_range(headers: &HeaderMap, meta: &Metadata) -> Result<Option<Range<u64>>> {
    let length = meta.geometry.length;
    if !range_requested(headers, meta) {
        return Ok(Some(0..length));
    }
    let text = policy::value(headers, "range")
        .unwrap()
        .trim_start_matches("bytes=");
    let Some((start, end)) = text.split_once('-') else {
        return Ok(Some(0..length));
    };
    if start.is_empty() {
        let suffix: u64 = match end.parse() {
            Ok(v) => v,
            Err(_) => return Ok(Some(0..length)),
        };
        return Ok((suffix > 0 && length > 0).then(|| length.saturating_sub(suffix)..length));
    }
    let start: u64 = match start.parse() {
        Ok(v) => v,
        Err(_) => return Ok(Some(0..length)),
    };
    let end = if end.is_empty() {
        length
    } else {
        match end.parse::<u64>() {
            Ok(v) => v.saturating_add(1).min(length),
            Err(_) => return Ok(Some(0..length)),
        }
    };
    Ok((start < end && start < length).then_some(start..end))
}
fn empty(status: StatusCode, headers: HeaderMap) -> Result<Response<ResponseBody>> {
    let mut response = Response::new(ResponseBody::Empty);
    *response.status_mut() = status;
    *response.headers_mut() = headers;
    if status != StatusCode::NOT_MODIFIED {
        response
            .headers_mut()
            .insert("content-length", "0".parse().unwrap());
    }
    Ok(response)
}
fn passthrough(
    response: reqwest::Response,
    permit: OwnedSemaphorePermit,
) -> Result<Response<ResponseBody>> {
    let status = response.status();
    let headers = policy::forwarded(response.headers());
    let body = response
        .bytes_stream()
        .map_ok(Frame::data)
        .map_err(HttpError::from);
    let body = futures_util::stream::unfold(
        (Box::pin(body), permit),
        |(mut stream, permit)| async move { stream.next().await.map(|frame| (frame, (stream, permit))) },
    );
    let mut response = Response::new(ResponseBody::Resource(BodyExt::boxed_unsync(
        StreamBody::new(body),
    )));
    *response.status_mut() = status;
    *response.headers_mut() = headers;
    Ok(response)
}
