use super::*;
#[path = "complete_file_tests.rs"]
mod complete_files;
#[path = "ffmpeg_test.rs"]
mod ffmpeg;
#[path = "live_segments_test.rs"]
mod live_segments;
use bytes::Bytes;
use http_body_util::BodyExt;
use reqwest::header::HeaderMap;
use std::{
    path::PathBuf,
    sync::{
        atomic::{AtomicBool, AtomicUsize, Ordering},
        Arc, Mutex,
    },
    time::Duration,
};
use tokio::io::{AsyncReadExt, AsyncWriteExt};

struct Directory(PathBuf);
impl Directory {
    fn new() -> Self {
        let path = std::env::temp_dir().join(format!("qcm-resource-{}", uuid::Uuid::new_v4()));
        std::fs::create_dir(&path).unwrap();
        Self(path)
    }
}
impl Drop for Directory {
    fn drop(&mut self) {
        let _ = std::fs::remove_dir_all(&self.0);
    }
}

fn meta(key: &str, length: u64) -> store::Metadata {
    store::Metadata {
        key: key.into(),
        provider: 1,
        kind: 1,
        generation: uuid::Uuid::new_v4().simple().to_string(),
        geometry: format::Geometry::new(length).unwrap(),
        headers: vec![],
        policy: policy::Policy::from_response(&HeaderMap::new(), policy::now(), policy::now()),
        target: "test".into(),
        ranges: true,
        layout: format::Layout::Blocks,
    }
}

#[test]
fn geometry_and_index_boundaries() {
    use format::{Geometry, Index};
    let geometry = Geometry::new(5 * 1024 * 1024 * 1024).unwrap();
    assert_eq!(geometry.block, 1024 * 1024);
    assert_eq!(geometry.count(), 5120);
    assert_eq!(Index::new(geometry).bytes().len(), 180 * 1024);
    assert_eq!(Geometry::new(0).unwrap().count(), 0);
    assert_eq!(
        geometry.offset(4096).unwrap(),
        format::HEADER_LEN + 4 * 1024 * 1024 * 1024
    );
    for (gib, block, count) in [
        (1, 256 * 1024, 4096),
        (2, 256 * 1024, 8192),
        (3, 512 * 1024, 6144),
        (4, 512 * 1024, 8192),
        (5, 1024 * 1024, 5120),
    ] {
        let geometry = Geometry::new(gib * 1024 * 1024 * 1024).unwrap();
        assert_eq!((geometry.block, geometry.count()), (block, count));
    }
}

#[tokio::test]
async fn unordered_slots_tail_corruption_and_restart() {
    let dir = Directory::new();
    let store = store::Store::open_blocks(dir.0.clone()).await.unwrap();
    let geometry = format::Geometry::new(format::MIN_BLOCK * 2 + 17).unwrap();
    let version = store
        .create(meta("resource", geometry.length))
        .await
        .unwrap();
    let a = Bytes::from(vec![1; format::MIN_BLOCK as usize]);
    let b = Bytes::from(vec![2; format::MIN_BLOCK as usize]);
    let tail = Bytes::from(vec![3; 17]);
    store
        .write(version.clone(), vec![(2, tail.clone()), (0, a.clone())])
        .await
        .unwrap();
    assert_eq!(store.read(version.clone(), 2).await.unwrap(), Some(tail));
    assert_eq!(store.read(version.clone(), 1).await.unwrap(), None);
    store
        .write(version.clone(), vec![(1, b.clone())])
        .await
        .unwrap();
    assert!(store.complete(version.clone()).await.unwrap());
    let path = dir.0.join(format!("{}.data", version.meta.generation));
    assert_eq!(
        std::fs::metadata(&path).unwrap().len(),
        format::HEADER_LEN + geometry.block * 3
    );
    store.checkpoint().await.unwrap();
    drop(version);
    drop(store);
    let store = store::Store::open_blocks(dir.0.clone()).await.unwrap();
    let version = store.lookup("resource".into()).await.unwrap().unwrap();
    assert_eq!(store.read(version.clone(), 0).await.unwrap(), Some(a));
    use std::io::{Seek, SeekFrom, Write};
    let mut file = std::fs::OpenOptions::new().write(true).open(path).unwrap();
    file.seek(SeekFrom::Start(format::HEADER_LEN + geometry.block * 2))
        .unwrap();
    file.write_all(&[9]).unwrap();
    assert_eq!(store.read(version.clone(), 1).await.unwrap(), None);
    assert!(store.read(version.clone(), 0).await.unwrap().is_some());
    store
        .write(version.clone(), vec![(1, b.clone())])
        .await
        .unwrap();
    assert_eq!(store.read(version, 1).await.unwrap(), Some(b));
}

#[tokio::test]
async fn failed_io_and_sql_do_not_publish_partial_indexes() {
    for fault in [store::Fault::Write, store::Fault::Sync, store::Fault::Sql] {
        let dir = Directory::new();
        let storage = store::Store::open(dir.0.clone()).await.unwrap();
        let version = storage
            .create(meta("failure", format::MIN_BLOCK * 3))
            .await
            .unwrap();
        let bytes = Bytes::from(vec![4; format::MIN_BLOCK as usize]);
        storage.fault(fault).await.unwrap();
        assert!(storage
            .write(
                version.clone(),
                vec![(0, bytes.clone()), (2, bytes.clone())]
            )
            .await
            .is_err());
        drop(version);
        drop(storage);
        let storage = store::Store::open(dir.0.clone()).await.unwrap();
        let version = storage.lookup("failure".into()).await.unwrap().unwrap();
        assert!(storage.read(version.clone(), 0).await.unwrap().is_none());
        assert!(storage.read(version.clone(), 2).await.unwrap().is_none());
        storage
            .write(
                version.clone(),
                vec![(0, bytes.clone()), (2, bytes.clone())],
            )
            .await
            .unwrap();
        assert_eq!(storage.read(version, 2).await.unwrap(), Some(bytes));
    }
}

#[tokio::test]
async fn restart_rejects_torn_files_and_corrupt_metadata() {
    let dir = Directory::new();
    let storage = store::Store::open_blocks(dir.0.clone()).await.unwrap();
    let version = storage
        .create(meta("torn", format::MIN_BLOCK))
        .await
        .unwrap();
    storage
        .write(
            version.clone(),
            vec![(0, Bytes::from(vec![1; format::MIN_BLOCK as usize]))],
        )
        .await
        .unwrap();
    let path = dir.0.join(format!("{}.data", version.meta.generation));
    drop(version);
    storage.shutdown().await.unwrap();
    drop(storage);
    std::fs::OpenOptions::new()
        .write(true)
        .open(path)
        .unwrap()
        .set_len(format::HEADER_LEN + 17)
        .unwrap();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    assert!(storage.lookup("torn".into()).await.unwrap().is_none());
    let version = storage.create(meta("bad-geometry", 10)).await.unwrap();
    drop(version);
    storage.shutdown().await.unwrap();
    drop(storage);
    let db = sqlite::Database::open(&dir.0.join("meta.sqlite")).unwrap();
    db.exec("UPDATE versions SET meta=json_set(meta,'$.geometry.block',0)")
        .unwrap();
    drop(db);
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    assert!(storage
        .lookup("bad-geometry".into())
        .await
        .unwrap()
        .is_none());
    storage.shutdown().await.unwrap();
}

#[test]
fn policies_respect_age_vary_and_directives() {
    let mut headers = HeaderMap::new();
    headers.insert("cache-control", "private, max-age=\"120\"".parse().unwrap());
    headers.insert("age", "90".parse().unwrap());
    headers.insert("etag", "\"one\"".parse().unwrap());
    let p = policy::Policy::from_response(&headers, policy::now(), policy::now());
    assert!(p.store && p.fresh(&HeaderMap::new()));
    let mut request = HeaderMap::new();
    request.insert("cache-control", "max-age=30".parse().unwrap());
    assert!(!p.fresh(&request));
    for directive in ["no-store", "must-understand"] {
        headers.insert("cache-control", directive.parse().unwrap());
        assert!(!policy::Policy::from_response(&headers, 0, 0).store);
    }
    headers.insert("cache-control", "no-cache, max-age=100".parse().unwrap());
    assert!(!policy::Policy::from_response(&headers, 0, 0).fresh(&HeaderMap::new()));
    headers.insert("vary", "*".parse().unwrap());
    assert!(!policy::Policy::from_response(&headers, 0, 0).store);
}

#[tokio::test]
async fn scopes_variants_and_only_if_cached_do_not_cross() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(5 * 1024 * 1024, true, false, Duration::ZERO).await;
    let mut missing = origin.request(None);
    missing
        .headers
        .insert("cache-control", "only-if-cached".parse().unwrap());
    assert_eq!(cache.response(missing).await.unwrap().status(), 504);
    assert!(origin.hits().is_empty());
    let small = cache
        .response(origin.request(Some("bytes=0-99")))
        .await
        .unwrap();
    assert_eq!(body(small).await, origin.expected(0, 100, 1));
    let mut missing = origin.request(Some("bytes=4500000-4500100"));
    missing
        .headers
        .insert("cache-control", "only-if-cached".parse().unwrap());
    let before = origin.hits().len();
    assert_eq!(cache.response(missing).await.unwrap().status(), 504);
    assert_eq!(origin.hits().len(), before);
    let mut other = origin.request(Some("bytes=0-99"));
    other.provider = 2;
    body(cache.response(other).await.unwrap()).await;
    assert!(origin.hits().len() > before);
    let mut other = origin.request(Some("bytes=0-99"));
    other.scope = "another account".into();
    let before = origin.hits().len();
    body(cache.response(other).await.unwrap()).await;
    assert!(origin.hits().len() > before);
    cache.invalidate(1).await.unwrap();
    let before = origin.hits().len();
    body(
        cache
            .response(origin.request(Some("bytes=0-99")))
            .await
            .unwrap(),
    )
    .await;
    assert!(origin.hits().len() > before);
    cache.shutdown().await.unwrap();
}

#[test]
#[ignore = "targeted SQLite write benchmark"]
fn blob_incremental_write_benchmark() {
    use sqlite::{Database, Value};
    for random in [false, true] {
        for incremental in [false, true] {
            let dir = Directory::new();
            let path = dir.0.join("meta.sqlite");
            let db = Database::open(&path).unwrap();
            db.exec("CREATE TABLE versions(id INTEGER PRIMARY KEY,blocks BLOB)")
                .unwrap();
            let mut bytes = vec![0u8; 8192 * format::ENTRY_LEN];
            db.statement("INSERT INTO versions VALUES(1,?)", &[Value::Blob(&bytes)])
                .unwrap()
                .next()
                .unwrap();
            db.exec("PRAGMA wal_checkpoint(TRUNCATE)").unwrap();
            db.exec("PRAGMA wal_autocheckpoint=0").unwrap();
            let started = std::time::Instant::now();
            for batch in 0..128 {
                let mut ranges = Vec::new();
                for index in 0..16 {
                    let block = if random {
                        (batch * 131 + index * 509) % 8192
                    } else {
                        batch * 16 + index
                    };
                    let start = block * format::ENTRY_LEN;
                    bytes[start..start + format::ENTRY_LEN].fill((batch + 1) as u8);
                    ranges.push(start..start + format::ENTRY_LEN);
                }
                db.exec("BEGIN IMMEDIATE").unwrap();
                if incremental {
                    db.patch(1, &bytes, &ranges).unwrap();
                } else {
                    db.statement(
                        "UPDATE versions SET blocks=? WHERE id=1",
                        &[Value::Blob(&bytes)],
                    )
                    .unwrap()
                    .next()
                    .unwrap();
                }
                db.exec("COMMIT").unwrap();
            }
            let wal = std::fs::metadata(dir.0.join("meta.sqlite-wal"))
                .unwrap()
                .len();
            let elapsed = started.elapsed().as_micros();
            let mut checkpoint = db.statement("PRAGMA wal_checkpoint(PASSIVE)", &[]).unwrap();
            checkpoint.next().unwrap();
            eprintln!("random={random} incremental={incremental} transactions=128 elapsed_us={elapsed} wal_bytes={wal} frames={} checkpointed={}",checkpoint.int(1),checkpoint.int(2));
        }
    }
}

#[test]
#[ignore = "subprocess crash helper"]
fn crash_worker() {
    let Some(root) = std::env::var_os("QCM_CACHE_CRASH_ROOT") else {
        return;
    };
    tokio::runtime::Runtime::new().unwrap().block_on(async {
        let storage = store::Store::open(PathBuf::from(root)).await.unwrap();
        let version = storage
            .create(meta("crash", format::MIN_BLOCK * 3))
            .await
            .unwrap();
        let fault = if std::env::var("QCM_CACHE_CRASH_POINT").unwrap() == "sync" {
            store::Fault::CrashSync
        } else {
            store::Fault::CrashSql
        };
        storage.fault(fault).await.unwrap();
        storage
            .write(
                version,
                vec![
                    (0, Bytes::from(vec![1; format::MIN_BLOCK as usize])),
                    (2, Bytes::from(vec![2; format::MIN_BLOCK as usize])),
                ],
            )
            .await
            .unwrap();
    });
    panic!("crash point was not reached");
}

#[test]
fn process_crash_recovers_without_publishing_uncommitted_blocks() {
    for point in ["sync", "sql"] {
        let dir = Directory::new();
        let output = std::process::Command::new(std::env::current_exe().unwrap())
            .args([
                "resource_cache::tests::crash_worker",
                "--exact",
                "--ignored",
            ])
            .env("QCM_CACHE_CRASH_ROOT", &dir.0)
            .env("QCM_CACHE_CRASH_POINT", point)
            .output()
            .unwrap();
        assert_eq!(
            output.status.code(),
            Some(42),
            "{}",
            String::from_utf8_lossy(&output.stderr)
        );
        tokio::runtime::Runtime::new().unwrap().block_on(async {
            let storage = store::Store::open(dir.0.clone()).await.unwrap();
            let version = storage.lookup("crash".into()).await.unwrap().unwrap();
            assert!(storage.read(version.clone(), 0).await.unwrap().is_none());
            assert!(storage.read(version.clone(), 2).await.unwrap().is_none());
            storage.shutdown().await.unwrap();
        });
    }
}

struct OriginState {
    hits: Mutex<Vec<HeaderMap>>,
    version: AtomicUsize,
    control: Mutex<String>,
    ranges: bool,
    ignore_ranges: AtomicBool,
    weak: bool,
    dates: AtomicBool,
    no_validator: AtomicBool,
    changed_date: AtomicBool,
    truncate: AtomicBool,
    unknown: AtomicBool,
    wrong_range: AtomicBool,
    status: AtomicUsize,
    delay: Duration,
    length: usize,
    payload: Mutex<Option<Arc<Vec<u8>>>>,
}
struct Origin {
    url: String,
    state: Arc<OriginState>,
    task: tokio::task::JoinHandle<()>,
}
impl Drop for Origin {
    fn drop(&mut self) {
        self.task.abort();
    }
}
impl Origin {
    async fn new(length: usize, ranges: bool, weak: bool, delay: Duration) -> Self {
        let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
        let url = format!("http://{}/resource", listener.local_addr().unwrap());
        let state = Arc::new(OriginState {
            hits: Mutex::new(vec![]),
            version: AtomicUsize::new(1),
            control: Mutex::new("private, max-age=3600".into()),
            ranges,
            ignore_ranges: AtomicBool::new(false),
            weak,
            dates: AtomicBool::new(false),
            no_validator: AtomicBool::new(false),
            changed_date: AtomicBool::new(false),
            truncate: AtomicBool::new(false),
            unknown: AtomicBool::new(false),
            wrong_range: AtomicBool::new(false),
            status: AtomicUsize::new(200),
            delay,
            length,
            payload: Mutex::new(None),
        });
        let shared = state.clone();
        let task = tokio::spawn(async move {
            while let Ok((mut stream, _)) = listener.accept().await {
                let state = shared.clone();
                tokio::spawn(async move {
                    let mut request = Vec::new();
                    let mut chunk = [0; 1024];
                    while !request.ends_with(b"\r\n\r\n") {
                        let n = stream.read(&mut chunk).await.unwrap_or(0);
                        if n == 0 || request.len() > 16 * 1024 {
                            return;
                        }
                        request.extend_from_slice(&chunk[..n]);
                    }
                    let mut headers = HeaderMap::new();
                    for line in String::from_utf8_lossy(&request).lines().skip(1) {
                        if let Some((name, value)) = line.split_once(':') {
                            if let (Ok(name), Ok(value)) = (
                                reqwest::header::HeaderName::from_bytes(name.trim().as_bytes()),
                                value.trim().parse::<reqwest::header::HeaderValue>(),
                            ) {
                                headers.append(name, value);
                            }
                        }
                    }
                    state.hits.lock().unwrap().push(headers.clone());
                    let version = state.version.load(Ordering::SeqCst);
                    let etag = format!("{}\"v{}\"", if state.weak { "W/" } else { "" }, version);
                    let modified = if state.changed_date.load(Ordering::SeqCst) {
                        "Wed, 21 Sep 2022 11:50:20 GMT"
                    } else {
                        "Wed, 21 Sep 2022 11:50:18 GMT"
                    };
                    let date_mode = state.dates.load(Ordering::SeqCst);
                    let validator = if date_mode { modified } else { etag.as_str() };
                    let mut status = state.status.load(Ordering::SeqCst);
                    let mut start = 0;
                    let mut end = state.length;
                    if status == 200
                        && headers
                            .get("if-none-match")
                            .is_some_and(|v| v == etag.as_str())
                        || (status == 200
                            && date_mode
                            && headers
                                .get("if-modified-since")
                                .is_some_and(|v| v == modified))
                    {
                        status = 304;
                    } else if status == 200
                        && state.ranges
                        && !state.ignore_ranges.load(Ordering::SeqCst)
                        && headers.get("if-range").is_none_or(|v| v == validator)
                    {
                        if let Some(range) = headers
                            .get("range")
                            .and_then(|v| v.to_str().ok())
                            .and_then(|s| s.strip_prefix("bytes="))
                        {
                            if let Some((a, b)) = range.split_once('-') {
                                if let Ok(a) = a.parse::<usize>() {
                                    start = a;
                                    end = if b.is_empty() {
                                        state.length
                                    } else {
                                        b.parse::<usize>().unwrap() + 1
                                    }
                                    .min(state.length);
                                } else if let Ok(suffix) = b.parse::<usize>() {
                                    start = state.length.saturating_sub(suffix);
                                }
                                status = if start >= end { 416 } else { 206 };
                            }
                        }
                    }
                    let length = if matches!(status, 200 | 206) {
                        end - start
                    } else {
                        0
                    };
                    let mut response=format!("HTTP/1.1 {status} OK\r\nContent-Type: audio/test\r\nContent-Length: {length}\r\nETag: {etag}\r\nCache-Control: {}\r\nConnection: close\r\n",state.control.lock().unwrap());
                    if date_mode || state.no_validator.load(Ordering::SeqCst) {
                        response = response.replace(&format!("ETag: {etag}\r\n"), "");
                    }
                    if date_mode {
                        response.push_str(&format!(
                            "Last-Modified: {modified}\r\nDate: Mon, 19 Oct 2026 07:35:21 GMT\r\n"
                        ));
                    }
                    if status == 304 {
                        response = response.replace("Content-Length: 0\r\n", "");
                    }
                    if state.unknown.load(Ordering::SeqCst) {
                        response = response.replace(&format!("Content-Length: {length}\r\n"), "");
                    }
                    if status == 206 {
                        response.push_str(&format!(
                            "Content-Range: bytes {start}-{}/{}\r\n",
                            if state.wrong_range.load(Ordering::SeqCst) {
                                state.length
                            } else {
                                end - 1
                            },
                            state.length
                        ));
                    }
                    if state.ranges {
                        response.push_str("Accept-Ranges: bytes\r\n");
                    }
                    response.push_str("\r\n");
                    if stream.write_all(response.as_bytes()).await.is_err() {
                        return;
                    }
                    let end = if state.truncate.load(Ordering::SeqCst) {
                        start + length / 2
                    } else {
                        start + length
                    };
                    while start < end {
                        let n = (end - start).min(64 * 1024);
                        let bytes: Vec<_> = state
                            .payload
                            .lock()
                            .unwrap()
                            .as_ref()
                            .map(|p| p[start..start + n].to_vec())
                            .unwrap_or_else(|| {
                                (start..start + n)
                                    .map(|i| ((i * 17 + version) % 251) as u8)
                                    .collect()
                            });
                        if stream.write_all(&bytes).await.is_err() {
                            break;
                        }
                        start += n;
                        if !state.delay.is_zero() {
                            tokio::time::sleep(state.delay).await;
                        }
                    }
                });
            }
        });
        Self { url, state, task }
    }
    fn request(&self, range: Option<&str>) -> ResourceRequest {
        let client = reqwest::Client::new();
        let request = client.get(&self.url).build().unwrap();
        let prepared =
            qcm_core::resource::PreparedResource::new(client, request, Duration::from_secs(5))
                .unwrap();
        let mut headers = HeaderMap::new();
        if let Some(range) = range {
            headers.insert("range", range.parse().unwrap());
        }
        ResourceRequest {
            provider: 1,
            kind: 1,
            scope: "account".into(),
            prepared: Arc::new(prepared),
            headers,
        }
    }
    fn expected(&self, start: usize, end: usize, version: usize) -> Bytes {
        Bytes::from(
            (start..end)
                .map(|i| ((i * 17 + version) % 251) as u8)
                .collect::<Vec<_>>(),
        )
    }
    fn hits(&self) -> Vec<HeaderMap> {
        self.state.hits.lock().unwrap().clone()
    }
}
async fn body(response: hyper::Response<crate::http::body_type::ResponseBody>) -> Bytes {
    tokio::time::timeout(Duration::from_secs(10), response.into_body().collect())
        .await
        .unwrap()
        .unwrap()
        .to_bytes()
}

#[tokio::test]
async fn transient_query_reuses_cache_and_keeps_target_validation() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(1024, true, false, Duration::ZERO).await;
    let signed = |salt: &str| {
        let mut request = origin.request(None);
        let client = reqwest::Client::new();
        let upstream = client
            .get(&origin.url)
            .query(&[("id", "song"), ("salt", salt)])
            .build()
            .unwrap();
        request.prepared = Arc::new(
            qcm_core::resource::PreparedResource::new(client, upstream, Duration::from_secs(5))
                .unwrap()
                .with_transient_query(vec!["salt".into()])
                .unwrap(),
        );
        request
    };
    assert_eq!(
        body(cache.response(signed("one")).await.unwrap()).await,
        origin.expected(0, 1024, 1)
    );
    let before = origin.hits().len();
    assert_eq!(
        body(cache.response(signed("two")).await.unwrap()).await,
        origin.expected(0, 1024, 1)
    );
    assert_eq!(origin.hits().len(), before);
    *origin.state.control.lock().unwrap() = "private, max-age=0".into();
    let mut request = signed("three");
    request
        .headers
        .insert("cache-control", "no-cache".parse().unwrap());
    assert_eq!(
        body(cache.response(request).await.unwrap()).await,
        origin.expected(0, 1024, 1)
    );
    assert!(origin.hits().len() > before);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn prepared_response_failure_does_not_publish_cache_data() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(1024, true, false, Duration::ZERO).await;
    let mut request = origin.request(None);
    let client = reqwest::Client::new();
    let upstream = client.get(&origin.url).build().unwrap();
    request.prepared = Arc::new(
        qcm_core::resource::PreparedResource::new(client, upstream, Duration::from_secs(5))
            .unwrap()
            .validated_by(|_| Box::pin(async { Err(qcm_core::error::ProviderError::NotFound) })),
    );
    assert!(cache.response(request).await.is_err());
    assert_eq!(cache.usage().await.unwrap().audio, 0);
    assert_eq!(origin.hits().len(), 1);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn range_reads_seek_suffix_and_cache_hit() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(6 * 1024 * 1024 + 13, true, false, Duration::ZERO).await;
    let response = cache
        .response(origin.request(Some("bytes=65537-400000")))
        .await
        .unwrap();
    assert_eq!(response.status(), 206);
    assert_eq!(response.headers()["content-length"], "334464");
    assert_eq!(body(response).await, origin.expected(65537, 400001, 1));
    assert_eq!(
        body(
            cache
                .response(origin.request(Some("bytes=5000000-5100000")))
                .await
                .unwrap()
        )
        .await,
        origin.expected(5000000, 5100001, 1)
    );
    let before = origin.hits().len();
    assert_eq!(
        body(
            cache
                .response(origin.request(Some("bytes=5000000-5100000")))
                .await
                .unwrap()
        )
        .await,
        origin.expected(5000000, 5100001, 1)
    );
    assert_eq!(origin.hits().len(), before);
    assert_eq!(
        body(
            cache
                .response(origin.request(Some("bytes=-17")))
                .await
                .unwrap()
        )
        .await,
        origin.expected(origin.state.length - 17, origin.state.length, 1)
    );
    let invalid = cache
        .response(origin.request(Some("bytes=99999999-")))
        .await
        .unwrap();
    assert_eq!(invalid.status(), 416);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn whole_resource_crosses_download_windows() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(5 * 1024 * 1024 + 7, true, false, Duration::ZERO).await;
    assert_eq!(
        body(cache.response(origin.request(None)).await.unwrap()).await,
        origin.expected(0, origin.state.length, 1)
    );
    let count = origin.hits().len();
    assert_eq!(
        body(cache.response(origin.request(None)).await.unwrap()).await,
        origin.expected(0, origin.state.length, 1)
    );
    assert_eq!(count, origin.hits().len());
    cache.shutdown().await.unwrap();
    drop(cache);
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    assert_eq!(
        body(
            cache
                .response(origin.request(Some("bytes=3-99")))
                .await
                .unwrap()
        )
        .await,
        origin.expected(3, 100, 1)
    );
    assert_eq!(count, origin.hits().len());
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn revalidation_304_and_new_generation() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(500000, true, false, Duration::ZERO).await;
    *origin.state.control.lock().unwrap() = "no-cache".into();
    assert_eq!(
        body(cache.response(origin.request(None)).await.unwrap()).await,
        origin.expected(0, 500000, 1)
    );
    assert_eq!(
        body(cache.response(origin.request(None)).await.unwrap()).await,
        origin.expected(0, 500000, 1)
    );
    assert!(origin.hits().last().unwrap().contains_key("if-none-match"));
    origin.state.version.store(2, Ordering::SeqCst);
    assert_eq!(
        body(cache.response(origin.request(None)).await.unwrap()).await,
        origin.expected(0, 500000, 2)
    );
    let mut request = origin.request(None);
    request
        .headers
        .insert("if-none-match", "\"v2\"".parse().unwrap());
    assert_eq!(cache.response(request).await.unwrap().status(), 304);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn no_store_and_unknown_capabilities_forward_without_cache() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(400000, true, false, Duration::ZERO).await;
    *origin.state.control.lock().unwrap() = "no-store".into();
    assert_eq!(
        body(
            cache
                .response(origin.request(Some("bytes=100-200")))
                .await
                .unwrap()
        )
        .await,
        origin.expected(100, 201, 1)
    );
    assert_eq!(cache.usage().await.unwrap().audio, 0);
    origin.state.status.store(404, Ordering::SeqCst);
    let response = cache.response(origin.request(None)).await.unwrap();
    assert_eq!(response.status(), 404);
    assert!(body(response).await.is_empty());
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn unknown_lengths_and_invalid_content_ranges() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(400000, false, false, Duration::ZERO).await;
    origin.state.unknown.store(true, Ordering::SeqCst);
    assert_eq!(
        body(cache.response(origin.request(None)).await.unwrap()).await,
        origin.expected(0, origin.state.length, 1)
    );
    assert_eq!(cache.usage().await.unwrap().audio, 0);
    let bad = Origin::new(400000, true, false, Duration::ZERO).await;
    bad.state.wrong_range.store(true, Ordering::SeqCst);
    assert!(matches!(
        cache.response(bad.request(None)).await,
        Err(crate::error::ProcessError::Resource(_))
    ));
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn resource_body_matches_http_wire_headers() {
    use hyper_util::rt::TokioIo;
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(600000, true, false, Duration::ZERO).await;
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    let url = format!("http://{}/image", listener.local_addr().unwrap());
    let request = origin.request(None);
    let upstream = request.prepared;
    let server_cache = cache.clone();
    let server = tokio::spawn(async move {
        for _ in 0..2 {
            let (socket, _) = listener.accept().await.unwrap();
            let cache = server_cache.clone();
            let prepared = upstream.clone();
            let service = hyper::service::service_fn(
                move |incoming: hyper::Request<hyper::body::Incoming>| {
                    let cache = cache.clone();
                    let prepared = prepared.clone();
                    async move {
                        cache
                            .response(ResourceRequest {
                                provider: 1,
                                kind: 1,
                                scope: "wire".into(),
                                prepared,
                                headers: incoming.headers().clone(),
                            })
                            .await
                    }
                },
            );
            hyper::server::conn::http1::Builder::new()
                .serve_connection(TokioIo::new(socket), service)
                .await
                .unwrap();
        }
    });
    let client = reqwest::Client::new();
    let response = client
        .get(&url)
        .header("range", "bytes=123-500000")
        .header("connection", "close")
        .send()
        .await
        .unwrap();
    assert_eq!(response.status(), 206);
    assert_eq!(response.content_length(), Some(499878));
    assert_eq!(
        response.bytes().await.unwrap(),
        origin.expected(123, 500001, 1)
    );
    let response = client
        .get(url)
        .header("range", "bytes=-99")
        .header("connection", "close")
        .send()
        .await
        .unwrap();
    assert_eq!(response.status(), 206);
    assert_eq!(
        response.bytes().await.unwrap(),
        origin.expected(599901, 600000, 1)
    );
    server.await.unwrap();
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn ignored_ranges_and_weak_validators_use_complete_response() {
    for (ranges, weak) in [(false, false), (true, true)] {
        let dir = Directory::new();
        let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
        let origin = Origin::new(1024 * 1024 + 11, ranges, weak, Duration::ZERO).await;
        assert_eq!(
            body(cache.response(origin.request(None)).await.unwrap()).await,
            origin.expected(0, origin.state.length, 1)
        );
        assert_eq!(
            body(
                cache
                    .response(origin.request(Some("bytes=100-777")))
                    .await
                    .unwrap()
            )
            .await,
            origin.expected(100, 778, 1)
        );
        cache.shutdown().await.unwrap();
    }
}

#[tokio::test]
async fn shared_download_survives_one_reader_cancelling() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(2 * 1024 * 1024, true, false, Duration::from_millis(2)).await;
    let a = cache.response(origin.request(None)).await.unwrap();
    let b = cache.response(origin.request(None)).await.unwrap();
    drop(a);
    assert_eq!(body(b).await, origin.expected(0, origin.state.length, 1));
    assert_eq!(origin.hits().len(), 1);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn truncation_terminates_body_and_shutdown_terminates_waiters() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(1024 * 1024, true, false, Duration::from_millis(1)).await;
    origin.state.truncate.store(true, Ordering::SeqCst);
    let response = cache.response(origin.request(None)).await.unwrap();
    assert!(
        tokio::time::timeout(Duration::from_secs(5), response.into_body().collect())
            .await
            .unwrap()
            .is_err()
    );
    origin.state.truncate.store(false, Ordering::SeqCst);
    let response = cache.response(origin.request(None)).await.unwrap();
    cache.shutdown().await.unwrap();
    assert!(
        tokio::time::timeout(Duration::from_secs(5), response.into_body().collect())
            .await
            .unwrap()
            .is_err()
    );
}

#[tokio::test]
async fn disk_failure_keeps_streaming_with_bounded_shared_buffer() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(7 * 1024 * 1024 + 9, false, false, Duration::from_millis(1)).await;
    cache
        .store
        .as_ref()
        .unwrap()
        .fault(store::Fault::Sync)
        .await
        .unwrap();
    assert_eq!(
        body(cache.response(origin.request(None)).await.unwrap()).await,
        origin.expected(0, origin.state.length, 1)
    );
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn slow_download_commits_completed_blocks_on_timer() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(2 * 1024 * 1024, true, false, Duration::from_millis(60)).await;
    let response = cache.response(origin.request(None)).await.unwrap();
    tokio::time::timeout(Duration::from_secs(2), async {
        while std::fs::read_dir(dir.0.join("resources-v1"))
            .unwrap()
            .filter_map(Result::ok)
            .filter(|e| e.path().extension().is_some_and(|s| s == "data"))
            .all(|e| e.metadata().unwrap().len() <= format::HEADER_LEN)
        {
            tokio::time::sleep(Duration::from_millis(10)).await;
        }
    })
    .await
    .unwrap();
    assert!(cache.usage().await.unwrap().audio < format::HEADER_LEN + 1024 * 1024);
    drop(response);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn reader_budget_and_provider_invalidation_terminate_active_reads() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(600000, true, false, Duration::ZERO).await;
    let mut readers = Vec::new();
    for _ in 0..16 {
        readers.push(cache.response(origin.request(None)).await.unwrap());
    }
    assert!(tokio::time::timeout(
        Duration::from_millis(20),
        cache.response(origin.request(None))
    )
    .await
    .is_err());
    drop(readers.pop());
    let response = cache.response(origin.request(None)).await.unwrap();
    cache.invalidate(1).await.unwrap();
    assert!(response.into_body().collect().await.is_err());
    drop(readers);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn malformed_ranges_and_empty_resources_do_not_overflow() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(0, false, false, Duration::ZERO).await;
    let response = cache
        .response(origin.request(Some("bytes=abc-def")))
        .await
        .unwrap();
    assert_eq!(response.status(), 200);
    assert!(body(response).await.is_empty());
    assert_eq!(
        cache
            .response(origin.request(Some("bytes=0-0")))
            .await
            .unwrap()
            .status(),
        200
    );
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn unavailable_storage_keeps_resource_requests_working() {
    let dir = Directory::new();
    let blocked = dir.0.join("blocked");
    std::fs::write(&blocked, b"not a directory").unwrap();
    let cache = ResourceCache::open(blocked).await.unwrap();
    let origin = Origin::new(100000, true, false, Duration::ZERO).await;
    assert_eq!(
        body(
            cache
                .response(origin.request(Some("bytes=5-99")))
                .await
                .unwrap()
        )
        .await,
        origin.expected(5, 100, 1)
    );
    assert_eq!(cache.usage().await.unwrap().audio, 0);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn changed_strong_version_refreshes_before_response_headers() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(6 * 1024 * 1024, true, false, Duration::from_millis(1)).await;
    body(
        cache
            .response(origin.request(Some("bytes=0-99")))
            .await
            .unwrap(),
    )
    .await;
    origin.state.version.store(2, Ordering::SeqCst);
    let response = cache
        .response(origin.request(Some("bytes=5000000-5100000")))
        .await
        .unwrap();
    assert_eq!(response.headers()["etag"], "\"v2\"");
    assert_eq!(body(response).await, origin.expected(5000000, 5100001, 2));
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn later_ignored_range_preserves_upstream_full_response() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(6 * 1024 * 1024, true, false, Duration::from_millis(1)).await;
    body(
        cache
            .response(origin.request(Some("bytes=0-99")))
            .await
            .unwrap(),
    )
    .await;
    origin.state.ignore_ranges.store(true, Ordering::SeqCst);
    let response = cache
        .response(origin.request(Some("bytes=5000000-5100000")))
        .await
        .unwrap();
    assert_eq!(response.status(), 200);
    assert!(!response.headers().contains_key("content-range"));
    let actual = body(response).await;
    assert!(actual == origin.expected(0, origin.state.length, 1));
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn missing_upstream_range_returns_404_before_response_headers() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(6 * 1024 * 1024, true, false, Duration::from_millis(1)).await;
    body(
        cache
            .response(origin.request(Some("bytes=0-99")))
            .await
            .unwrap(),
    )
    .await;
    origin.state.status.store(404, Ordering::SeqCst);
    let before = origin.hits().len();
    let response = cache
        .response(origin.request(Some("bytes=5000000-5100000")))
        .await
        .unwrap();
    assert_eq!(response.status(), 404);
    assert!(body(response).await.is_empty());
    assert_eq!(origin.hits().len(), before + 1);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn weak_version_corruption_does_not_mix_new_and_old_blocks() {
    use std::io::{Seek, SeekFrom, Write};
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    cache
        .store
        .as_ref()
        .unwrap()
        .test_materialization(false)
        .await
        .unwrap();
    let origin = Origin::new(600000, true, true, Duration::ZERO).await;
    body(cache.response(origin.request(None)).await.unwrap()).await;
    let path = std::fs::read_dir(dir.0.join("resources-v1"))
        .unwrap()
        .filter_map(Result::ok)
        .find(|e| e.path().extension().is_some_and(|s| s == "data"))
        .unwrap()
        .path();
    let mut file = std::fs::OpenOptions::new().write(true).open(path).unwrap();
    file.seek(SeekFrom::Start(format::HEADER_LEN + format::MIN_BLOCK))
        .unwrap();
    file.write_all(&[255]).unwrap();
    drop(file);
    origin.state.version.store(2, Ordering::SeqCst);
    let response = cache.response(origin.request(None)).await.unwrap();
    assert!(response.into_body().collect().await.is_err());
    assert_eq!(
        body(cache.response(origin.request(None)).await.unwrap()).await,
        origin.expected(0, 600000, 2)
    );
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn date_ranges_seek_across_segments_revalidate_and_preserve_if_range() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(
        (format::SEGMENT_BYTES * 3 + 17) as usize,
        true,
        false,
        Duration::ZERO,
    )
    .await;
    origin.state.dates.store(true, Ordering::SeqCst);
    for start in [format::SEGMENT_BYTES - 123, format::SEGMENT_BYTES * 2 + 123] {
        let response = cache
            .response(origin.request(Some(&format!("bytes={start}-{}", start + 999))))
            .await
            .unwrap();
        assert_eq!(response.status(), 206);
        assert!(body(response).await == origin.expected(start as usize, start as usize + 1000, 1));
    }
    let hits = origin.hits();
    assert!(hits.iter().all(|h| h
        .get("range")
        .is_some_and(|v| !v.to_str().unwrap().starts_with("bytes=0-"))));
    assert!(hits.iter().any(|h| h
        .get("if-range")
        .is_some_and(|v| v == "Wed, 21 Sep 2022 11:50:18 GMT")));
    let mut req = origin.request(Some("bytes=100-199"));
    req.headers
        .insert("if-range", "Thu, 01 Jan 1970 00:00:00 GMT".parse().unwrap());
    let response = cache.response(req).await.unwrap();
    assert_eq!(response.status(), 200);
    drop(response);
    cache.shutdown().await.unwrap();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let start = format::SEGMENT_BYTES * 2 + 123;
    let mut req = origin.request(Some(&format!("bytes={start}-{}", start + 999)));
    req.headers
        .insert("cache-control", "only-if-cached".parse().unwrap());
    assert!(
        body(cache.response(req).await.unwrap()).await
            == origin.expected(start as usize, start as usize + 1000, 1)
    );
    // A changed date must not be combined into an already cached representation.
    origin.state.changed_date.store(true, Ordering::SeqCst);
    let response = cache
        .response(origin.request(Some("bytes=40000000-40000100")))
        .await
        .unwrap();
    assert_eq!(response.status(), 206);
    assert_eq!(
        response.headers()["last-modified"],
        "Wed, 21 Sep 2022 11:50:20 GMT"
    );
    drop(response);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn unvalidated_cold_seek_passthrough_never_fetches_prefix_or_persists() {
    for weak in [true, false] {
        let dir = Directory::new();
        let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
        let origin = Origin::new(64 * 1024 * 1024, true, weak, Duration::ZERO).await;
        origin.state.no_validator.store(!weak, Ordering::SeqCst);
        let response = cache
            .response(origin.request(Some("bytes=40000000-40000511")))
            .await
            .unwrap();
        assert_eq!(response.status(), 206);
        assert!(body(response).await == origin.expected(40000000, 40000512, 1));
        assert!(origin.hits().iter().all(|h| h
            .get("range")
            .is_some_and(|v| !v.to_str().unwrap().starts_with("bytes=0-"))));
        assert_eq!(cache.usage().await.unwrap().audio, 0);
        cache.shutdown().await.unwrap();
    }
}

#[tokio::test]
async fn complete_long_audio_stays_segmented_without_plain_copy() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(
        (format::SEGMENT_BYTES + 17) as usize,
        true,
        false,
        Duration::ZERO,
    )
    .await;
    let actual = body(cache.response(origin.request(None)).await.unwrap()).await;
    assert!(actual == origin.expected(0, origin.state.length, 1));
    tokio::time::sleep(Duration::from_millis(1200)).await;
    let names: Vec<_> = std::fs::read_dir(dir.0.join("resources-v1"))
        .unwrap()
        .map(|e| e.unwrap().file_name().to_string_lossy().into_owned())
        .collect();
    assert_eq!(names.iter().filter(|s| s.ends_with(".seg")).count(), 2);
    assert!(!names
        .iter()
        .any(|s| s.ends_with(".ok") || s.ends_with(".ok.tmp") || s.ends_with(".data")));
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn date_revalidation_304_retains_original_evidence_and_rejects_changed_date() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(20 * 1024 * 1024, true, false, Duration::ZERO).await;
    origin.state.dates.store(true, Ordering::SeqCst);
    *origin.state.control.lock().unwrap() = "private, no-cache".into();
    body(
        cache
            .response(origin.request(Some("bytes=10000000-10000511")))
            .await
            .unwrap(),
    )
    .await;
    let before = origin.hits().len();
    let response = cache
        .response(origin.request(Some("bytes=10000000-10000511")))
        .await
        .unwrap();
    assert_eq!(response.status(), 206);
    assert!(body(response).await == origin.expected(10000000, 10000512, 1));
    assert_eq!(origin.hits().len(), before + 1);
    assert!(origin
        .hits()
        .last()
        .unwrap()
        .contains_key("if-modified-since"));
    origin.state.status.store(304, Ordering::SeqCst);
    origin.state.changed_date.store(true, Ordering::SeqCst);
    assert!(cache
        .response(origin.request(Some("bytes=10000000-10000511")))
        .await
        .is_err());
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn segmented_far_seek_does_not_wait_for_sequential_reader() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(24 * 1024 * 1024, true, false, Duration::from_millis(10)).await;
    let sequential = cache.response(origin.request(None)).await.unwrap();
    let seek = tokio::time::timeout(
        Duration::from_secs(1),
        cache.response(origin.request(Some("bytes=20000000-20000511"))),
    )
    .await
    .unwrap()
    .unwrap();
    assert!(body(seek).await == origin.expected(20000000, 20000512, 1));
    cache.invalidate(1).await.unwrap();
    assert!(sequential.into_body().collect().await.is_err());
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn unvalidated_seek_does_not_join_incomplete_full_download() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(24 * 1024 * 1024, true, true, Duration::from_millis(10)).await;
    let sequential = cache.response(origin.request(None)).await.unwrap();
    let before = origin.hits().len();
    let seek = tokio::time::timeout(
        Duration::from_secs(1),
        cache.response(origin.request(Some("bytes=20000000-20000511"))),
    )
    .await
    .unwrap()
    .unwrap();
    assert_eq!(seek.status(), 206);
    assert!(body(seek).await == origin.expected(20000000, 20000512, 1));
    assert!(origin.hits()[before..].iter().all(|h| h
        .get("range")
        .is_some_and(|v| !v.to_str().unwrap().starts_with("bytes=0-"))));
    drop(sequential);
    cache.shutdown().await.unwrap();
}
