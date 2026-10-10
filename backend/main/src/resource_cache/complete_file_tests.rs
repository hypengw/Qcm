use super::*;
use std::io::{Seek, SeekFrom, Write};

async fn wait_ready(storage: &store::Store, version: &store::Version) {
    tokio::time::timeout(Duration::from_secs(8), async {
        while !storage.plain_ready(version.clone()).await.unwrap() {
            tokio::time::sleep(Duration::from_millis(10)).await;
        }
    })
    .await
    .unwrap();
}
async fn wait_deferred(storage: &store::Store, version: &store::Version) {
    tokio::time::timeout(Duration::from_secs(5), async {
        while !storage.test_deferred(version.clone()).await.unwrap() {
            tokio::time::sleep(Duration::from_millis(10)).await;
        }
    })
    .await
    .unwrap();
}
async fn write_all(storage: &store::Store, version: &store::Version, data: &[u8]) {
    let blocks: Vec<_> = data
        .chunks(version.meta.geometry.block as usize)
        .enumerate()
        .rev()
        .map(|(i, data)| (i as u64, Bytes::copy_from_slice(data)))
        .collect();
    assert!(
        storage
            .write(version.clone(), blocks)
            .await
            .unwrap()
            .complete
    );
}
fn image_meta(key: &str, length: u64) -> store::Metadata {
    let mut m = meta(key, length);
    m.kind = 0;
    m
}

#[tokio::test]
async fn unordered_complete_file_has_exact_bytes_and_survives_restart() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let data: Vec<_> = (0..format::MIN_BLOCK * 2 + 17)
        .map(|i| (i % 251) as u8)
        .collect();
    let version = storage
        .create(meta("plain", data.len() as u64))
        .await
        .unwrap();
    write_all(&storage, &version, &data).await;
    wait_ready(&storage, &version).await;
    let path = dir.0.join(format!("{}.ok", version.meta.generation));
    assert_eq!(std::fs::read(&path).unwrap(), data);
    assert!(!dir
        .0
        .join(format!("{}.data", version.meta.generation))
        .exists());
    assert!(!dir
        .0
        .join(format!("{}.ok.tmp", version.meta.generation))
        .exists());
    assert_eq!(
        storage
            .read(version.clone(), 2)
            .await
            .unwrap()
            .unwrap()
            .as_ref(),
        &data[data.len() - 17..]
    );
    let usage = storage.usage().await.unwrap();
    assert!(usage.audio >= data.len() as u64 && usage.audio < data.len() as u64 + 8192);
    storage.shutdown().await.unwrap();
    drop(storage);
    drop(version);
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.lookup("plain".into()).await.unwrap().unwrap();
    assert_eq!(
        storage
            .read(version.clone(), 0)
            .await
            .unwrap()
            .unwrap()
            .as_ref(),
        &data[..format::MIN_BLOCK as usize]
    );
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn empty_file_is_published_without_blocks() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.create(meta("empty", 0)).await.unwrap();
    wait_ready(&storage, &version).await;
    assert_eq!(
        std::fs::metadata(dir.0.join(format!("{}.ok", version.meta.generation)))
            .unwrap()
            .len(),
        0
    );
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn cache_directory_has_one_storage_owner() {
    let dir = Directory::new();
    let storage = store::Store::open_blocks(dir.0.clone()).await.unwrap();
    let version = storage.create(meta("owner", 7)).await.unwrap();
    write_all(&storage, &version, &[2; 7]).await;
    assert!(store::Store::open(dir.0.clone()).await.is_err());
    assert_eq!(
        storage
            .read(version.clone(), 0)
            .await
            .unwrap()
            .unwrap()
            .as_ref(),
        &[2; 7]
    );
    storage.shutdown().await.unwrap();
    drop(storage);
    drop(version);
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.lookup("owner".into()).await.unwrap().unwrap();
    wait_ready(&storage, &version).await;
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn published_file_url_round_trips_non_ascii_cache_paths() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.join("space # 中文"))
        .await
        .unwrap();
    let version = storage.create(image_meta("url", 7)).await.unwrap();
    write_all(&storage, &version, &[6; 7]).await;
    wait_ready(&storage, &version).await;
    let file = storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .unwrap();
    let token = file.token.clone();
    let path = file.path.clone();
    let response = ResourceDelivery::LocalFile(file).into_response().unwrap();
    let location = response.headers()["location"].to_str().unwrap();
    assert!(location.contains("%20") && location.contains("%23"));
    assert_eq!(
        reqwest::Url::parse(location)
            .unwrap()
            .to_file_path()
            .unwrap(),
        path
    );
    storage.release_file(token, "client".into()).await.unwrap();
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn duplicate_complete_batches_publish_only_one_file() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage
        .create(meta("duplicate", format::MIN_BLOCK * 2))
        .await
        .unwrap();
    let blocks = vec![
        (1, Bytes::from(vec![2; format::MIN_BLOCK as usize])),
        (0, Bytes::from(vec![1; format::MIN_BLOCK as usize])),
    ];
    let mut tasks = tokio::task::JoinSet::new();
    for _ in 0..16 {
        let storage = storage.clone();
        let version = version.clone();
        let blocks = blocks.clone();
        tasks.spawn(async move {
            storage.write(version, blocks).await.unwrap();
        });
    }
    while let Some(result) = tasks.join_next().await {
        result.unwrap();
    }
    wait_ready(&storage, &version).await;
    assert_eq!(
        storage.read(version.clone(), 0).await.unwrap().unwrap()[0],
        1
    );
    assert_eq!(
        storage.read(version.clone(), 1).await.unwrap().unwrap()[0],
        2
    );
    let names: Vec<_> = std::fs::read_dir(&dir.0)
        .unwrap()
        .map(|e| e.unwrap().file_name().to_string_lossy().into_owned())
        .filter(|s| s.starts_with(&version.meta.generation))
        .collect();
    assert_eq!(names, vec![format!("{}.ok", version.meta.generation)]);
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn uncommitted_complete_index_never_publishes() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.create(meta("sql", 100)).await.unwrap();
    storage.fault(store::Fault::Sql).await.unwrap();
    assert!(storage
        .write(version.clone(), vec![(0, Bytes::from(vec![7; 100]))])
        .await
        .is_err());
    assert!(!storage.complete(version.clone()).await.unwrap());
    tokio::time::sleep(Duration::from_millis(150)).await;
    assert!(!storage.plain_ready(version.clone()).await.unwrap());
    assert!(
        storage
            .write(version.clone(), vec![])
            .await
            .unwrap()
            .complete
    );
    wait_ready(&storage, &version).await;
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn conversion_failures_preserve_blocks_and_can_retry() {
    for fault in [
        store::Fault::MaterializeWrite,
        store::Fault::MaterializeSync,
        store::Fault::MaterializeRename,
        store::Fault::MaterializeSql,
    ] {
        let dir = Directory::new();
        let storage = store::Store::open(dir.0.clone()).await.unwrap();
        let version = storage.create(meta("failure", 123)).await.unwrap();
        storage.fault(fault).await.unwrap();
        write_all(&storage, &version, &[8; 123]).await;
        wait_deferred(&storage, &version).await;
        assert!(!storage.plain_ready(version.clone()).await.unwrap());
        assert_eq!(
            storage
                .read(version.clone(), 0)
                .await
                .unwrap()
                .unwrap()
                .as_ref(),
            &[8; 123]
        );
        assert!(!dir
            .0
            .join(format!("{}.ok", version.meta.generation))
            .exists());
        storage.test_retry(version.clone()).await.unwrap();
        wait_ready(&storage, &version).await;
        storage.shutdown().await.unwrap();
    }
}

#[tokio::test]
async fn corrupt_block_prevents_file_publication() {
    let dir = Directory::new();
    let storage = store::Store::open_blocks(dir.0.clone()).await.unwrap();
    let version = storage.create(meta("bad-block", 99)).await.unwrap();
    write_all(&storage, &version, &[9; 99]).await;
    let path = dir.0.join(format!("{}.data", version.meta.generation));
    let mut file = std::fs::OpenOptions::new().write(true).open(&path).unwrap();
    file.seek(SeekFrom::Start(format::HEADER_LEN)).unwrap();
    file.write_all(&[3]).unwrap();
    drop(file);
    storage.test_materialization(true).await.unwrap();
    wait_deferred(&storage, &version).await;
    assert!(!storage.complete(version.clone()).await.unwrap());
    assert!(storage.read(version.clone(), 0).await.unwrap().is_none());
    write_all(&storage, &version, &[9; 99]).await;
    storage.test_retry(version.clone()).await.unwrap();
    wait_ready(&storage, &version).await;
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn plain_corruption_retires_instead_of_overwriting() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.create(image_meta("bad-file", 99)).await.unwrap();
    write_all(&storage, &version, &[9; 99]).await;
    wait_ready(&storage, &version).await;
    let file = storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .unwrap();
    let mut writer = std::fs::OpenOptions::new()
        .write(true)
        .open(&file.path)
        .unwrap();
    writer.write_all(&[3]).unwrap();
    drop(writer);
    assert!(storage
        .read(version.clone(), 0)
        .await
        .unwrap_err()
        .is::<store::PlainCorrupt>());
    assert!(!version.is_valid());
    assert!(storage.lookup("bad-file".into()).await.unwrap().is_none());
    assert!(storage
        .write(version.clone(), vec![(0, Bytes::from(vec![9; 99]))])
        .await
        .is_err());
    assert!(file.path.exists());
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn capacity_defers_conversion_without_losing_cached_bytes() {
    let dir = Directory::new();
    let storage = store::Store::open_blocks(dir.0.clone()).await.unwrap();
    let version = storage
        .create(meta("space", format::MIN_BLOCK))
        .await
        .unwrap();
    write_all(&storage, &version, &vec![4; format::MIN_BLOCK as usize]).await;
    let usage = storage.usage().await.unwrap();
    storage
        .test_capacity(usage.audio + usage.metadata + 32768)
        .await
        .unwrap();
    storage.test_materialization(true).await.unwrap();
    wait_deferred(&storage, &version).await;
    assert!(storage.complete(version.clone()).await.unwrap());
    assert!(storage.read(version.clone(), 0).await.unwrap().is_some());
    storage.test_capacity(10 * 1024 * 1024).await.unwrap();
    storage.test_retry(version.clone()).await.unwrap();
    wait_ready(&storage, &version).await;
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn failed_data_cleanup_keeps_plain_readable_and_counts_both_files() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage
        .create(meta("cleanup", format::MIN_BLOCK))
        .await
        .unwrap();
    storage
        .fault(store::Fault::MaterializeDelete)
        .await
        .unwrap();
    write_all(&storage, &version, &vec![5; format::MIN_BLOCK as usize]).await;
    wait_ready(&storage, &version).await;
    assert!(dir
        .0
        .join(format!("{}.data", version.meta.generation))
        .exists());
    assert!(storage.usage().await.unwrap().audio >= 2 * format::MIN_BLOCK);
    assert_eq!(
        storage.read(version.clone(), 0).await.unwrap().unwrap()[0],
        5
    );
    storage.clear_fault().await.unwrap();
    tokio::time::timeout(Duration::from_secs(5), async {
        while dir
            .0
            .join(format!("{}.data", version.meta.generation))
            .exists()
        {
            tokio::time::sleep(Duration::from_millis(10)).await;
        }
    })
    .await
    .unwrap();
    assert!(storage.usage().await.unwrap().audio < format::MIN_BLOCK + 8192);
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn external_lease_protects_file_across_invalidation_and_restart() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.create(image_meta("lease", 500)).await.unwrap();
    write_all(&storage, &version, &[5; 500]).await;
    wait_ready(&storage, &version).await;
    let local = storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .unwrap();
    assert!(!storage
        .renew_file(local.token.clone(), "other".into())
        .await
        .unwrap());
    assert!(storage
        .renew_file(local.token.clone(), "client".into())
        .await
        .unwrap());
    storage.invalidate(1).await.unwrap();
    assert!(storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .is_none());
    assert!(!storage
        .renew_file(local.token.clone(), "client".into())
        .await
        .unwrap());
    drop(version);
    storage.shutdown().await.unwrap();
    drop(storage);
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    assert!(local.path.exists());
    storage
        .release_file(local.token.clone(), "other".into())
        .await
        .unwrap();
    assert!(local.path.exists());
    storage
        .release_file(local.token, "client".into())
        .await
        .unwrap();
    assert!(!local.path.exists());
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn expired_file_lease_cannot_be_renewed() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.create(image_meta("expires", 7)).await.unwrap();
    write_all(&storage, &version, &[5; 7]).await;
    wait_ready(&storage, &version).await;
    let local = storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .unwrap();
    storage.test_expire(local.token.clone()).await.unwrap();
    assert!(!storage
        .renew_file(local.token, "client".into())
        .await
        .unwrap());
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn file_delivery_requires_capability_and_client_authorization() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(600000, true, false, Duration::ZERO).await;
    let mut req = origin.request(None);
    req.kind = 0;
    assert_eq!(
        body(cache.response(req.clone()).await.unwrap()).await,
        origin.expected(0, 600000, 1)
    );
    tokio::time::timeout(Duration::from_secs(8), async {
        while !std::fs::read_dir(dir.0.join("resources-v1"))
            .unwrap()
            .any(|e| e.unwrap().path().extension().is_some_and(|s| s == "ok"))
        {
            tokio::time::sleep(Duration::from_millis(10)).await;
        }
    })
    .await
    .unwrap();
    let key = uuid::Uuid::new_v4().simple().to_string();
    req.headers
        .insert("qextra-image-transport", "file-lease-v1".parse().unwrap());
    req.headers.insert("qcm-file-client", key.parse().unwrap());
    assert_eq!(cache.response(req.clone()).await.unwrap().status(), 200);
    cache.authorize_file_client(&key).unwrap();
    let response = cache.response(req.clone()).await.unwrap();
    assert_eq!(response.status(), 302);
    assert_eq!(response.headers()["cache-control"], "no-store");
    let path = reqwest::Url::parse(response.headers()["location"].to_str().unwrap())
        .unwrap()
        .to_file_path()
        .unwrap();
    assert_eq!(std::fs::read(path).unwrap(), origin.expected(0, 600000, 1));
    let renew = response.headers()["qcm-file-renew"].to_str().unwrap();
    let release = response.headers()["qcm-file-release"].to_str().unwrap();
    assert_eq!(
        cache
            .file_lease_response(&hyper::Method::POST, renew, &HeaderMap::new())
            .await
            .unwrap()
            .status(),
        403
    );
    assert_eq!(
        cache
            .file_lease_response(&hyper::Method::POST, renew, &req.headers)
            .await
            .unwrap()
            .status(),
        204
    );
    assert_eq!(
        cache
            .file_lease_response(&hyper::Method::DELETE, release, &req.headers)
            .await
            .unwrap()
            .status(),
        204
    );
    assert_eq!(
        cache
            .file_lease_response(&hyper::Method::POST, renew, &req.headers)
            .await
            .unwrap()
            .status(),
        410
    );
    req.headers
        .insert("if-none-match", "\"v1\"".parse().unwrap());
    assert_eq!(cache.response(req.clone()).await.unwrap().status(), 304);
    req.headers.remove("if-none-match");
    req.headers
        .insert("cache-control", "no-store".parse().unwrap());
    let response = cache.response(req.clone()).await.unwrap();
    assert_eq!(response.status(), 200);
    assert!(!response.headers().contains_key("location"));
    body(response).await;
    req.headers.remove("cache-control");
    req.headers
        .insert("cache-control", "no-cache".parse().unwrap());
    let before = origin.hits().len();
    assert_eq!(cache.response(req.clone()).await.unwrap().status(), 302);
    assert!(origin.hits().len() > before);
    assert!(origin.hits().last().unwrap().contains_key("if-none-match"));
    assert!(origin
        .hits()
        .iter()
        .all(|h| !h.contains_key("qcm-file-client") && !h.contains_key("qextra-image-transport")));
    req.headers.remove("cache-control");
    req.headers.insert("range", "bytes=0-9".parse().unwrap());
    assert_eq!(cache.response(req.clone()).await.unwrap().status(), 206);
    req.headers.remove("range");
    req.kind = 1;
    assert_eq!(cache.response(req).await.unwrap().status(), 200);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn restart_grace_does_not_resurrect_expired_tokens() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage
        .create(image_meta("expired-restart", 7))
        .await
        .unwrap();
    write_all(&storage, &version, &[4; 7]).await;
    wait_ready(&storage, &version).await;
    let local = storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .unwrap();
    storage.invalidate(1).await.unwrap();
    storage.shutdown().await.unwrap();
    drop(storage);
    drop(version);
    let db = sqlite::Database::open(&dir.0.join("meta.sqlite")).unwrap();
    db.exec("UPDATE file_leases SET expires=0").unwrap();
    drop(db);
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    assert!(local.path.exists());
    assert!(!storage
        .renew_file(local.token.clone(), "client".into())
        .await
        .unwrap());
    storage
        .release_file(local.token, "client".into())
        .await
        .unwrap();
    assert!(!local.path.exists());
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn clock_rollback_refuses_new_delivery_but_retains_files() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.create(image_meta("clock", 7)).await.unwrap();
    write_all(&storage, &version, &[4; 7]).await;
    wait_ready(&storage, &version).await;
    let local = storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .unwrap();
    storage.shutdown().await.unwrap();
    drop(storage);
    drop(version);
    let db = sqlite::Database::open(&dir.0.join("meta.sqlite")).unwrap();
    db.exec(&format!(
        "UPDATE file_clock SET time={}",
        policy::now() + 3600
    ))
    .unwrap();
    drop(db);
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.lookup("clock".into()).await.unwrap().unwrap();
    assert!(local.path.exists());
    assert!(storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .is_none());
    assert!(!storage
        .renew_file(local.token.clone(), "client".into())
        .await
        .unwrap());
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn lease_count_is_bounded_and_release_is_idempotent() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.create(image_meta("budget", 7)).await.unwrap();
    write_all(&storage, &version, &[4; 7]).await;
    wait_ready(&storage, &version).await;
    let mut tokens = Vec::new();
    for _ in 0..256 {
        tokens.push(
            storage
                .local_file(version.clone(), "client".into())
                .await
                .unwrap()
                .unwrap()
                .token,
        );
    }
    assert!(storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .is_none());
    let token = tokens.pop().unwrap();
    storage
        .release_file(token.clone(), "client".into())
        .await
        .unwrap();
    storage.release_file(token, "client".into()).await.unwrap();
    assert!(storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .is_some());
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn orphan_files_are_recovered_without_touching_unmanaged_paths() {
    let dir = Directory::new();
    let storage = store::Store::open_blocks(dir.0.clone()).await.unwrap();
    let version = storage.create(meta("orphan", 7)).await.unwrap();
    write_all(&storage, &version, &[2; 7]).await;
    storage.shutdown().await.unwrap();
    drop(storage);
    for suffix in ["ok.tmp", "ok"] {
        let path = dir.0.join(format!("{}.{suffix}", version.meta.generation));
        std::fs::File::create(path)
            .unwrap()
            .write_all(b"unpublished")
            .unwrap();
    }
    let unrelated = dir.0.join("user.ok");
    std::fs::File::create(&unrelated)
        .unwrap()
        .write_all(b"user")
        .unwrap();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    wait_ready(&storage, &version).await;
    assert_eq!(
        std::fs::read(dir.0.join(format!("{}.ok", version.meta.generation))).unwrap(),
        [2; 7]
    );
    assert_eq!(std::fs::read(unrelated).unwrap(), b"user");
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn old_blocks_schema_migrates_without_discarding_bytes() {
    let dir = Directory::new();
    let storage = store::Store::open_blocks(dir.0.clone()).await.unwrap();
    let version = storage.create(meta("old", 19)).await.unwrap();
    write_all(&storage, &version, &[8; 19]).await;
    storage.shutdown().await.unwrap();
    drop(storage);
    drop(version);
    let db = sqlite::Database::open(&dir.0.join("meta.sqlite")).unwrap();
    db.exec("DROP TABLE complete_files; DROP TABLE file_leases; DROP TABLE file_clock; PRAGMA user_version=0;").unwrap();
    drop(db);
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.lookup("old".into()).await.unwrap().unwrap();
    wait_ready(&storage, &version).await;
    assert_eq!(
        storage
            .read(version.clone(), 0)
            .await
            .unwrap()
            .unwrap()
            .as_ref(),
        &[8; 19]
    );
    storage.shutdown().await.unwrap();
}

#[tokio::test]
async fn missing_plain_file_retires_without_using_old_data() {
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage.create(meta("missing", 7)).await.unwrap();
    write_all(&storage, &version, &[2; 7]).await;
    wait_ready(&storage, &version).await;
    storage.shutdown().await.unwrap();
    drop(storage);
    std::fs::remove_file(dir.0.join(format!("{}.ok", version.meta.generation))).unwrap();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    assert!(storage.lookup("missing".into()).await.unwrap().is_none());
    storage.shutdown().await.unwrap();
}

#[test]
#[ignore]
fn complete_file_crash_worker() {
    let Ok(root) = std::env::var("QCM_COMPLETE_CRASH_ROOT") else {
        return;
    };
    tokio::runtime::Runtime::new().unwrap().block_on(async {
        let storage = store::Store::open(PathBuf::from(root)).await.unwrap();
        let fault = match std::env::var("QCM_COMPLETE_CRASH_POINT").unwrap().as_str() {
            "sync" => store::Fault::CrashMaterializeSync,
            "rename" => store::Fault::CrashMaterializeRename,
            "sql" => store::Fault::CrashMaterializeSql,
            "create" => store::Fault::CrashMaterializeCreate,
            "write" => store::Fault::CrashMaterializeWrite,
            "directory" => store::Fault::CrashMaterializeDirectory,
            "delete" => store::Fault::CrashMaterializeDelete,
            _ => unreachable!(),
        };
        let version = storage.create(meta("crash", 700000)).await.unwrap();
        storage.fault(fault).await.unwrap();
        write_all(&storage, &version, &vec![6; 700000]).await;
        wait_ready(&storage, &version).await;
        panic!("crash point was not reached");
    });
}

#[test]
fn crash_during_file_publication_recovers_only_committed_layout() {
    for point in [
        "create",
        "write",
        "sync",
        "directory",
        "rename",
        "sql",
        "delete",
    ] {
        let dir = Directory::new();
        let status = std::process::Command::new(std::env::current_exe().unwrap())
            .args([
                "resource_cache::tests::complete_files::complete_file_crash_worker",
                "--exact",
                "--ignored",
            ])
            .env("QCM_COMPLETE_CRASH_ROOT", &dir.0)
            .env("QCM_COMPLETE_CRASH_POINT", point)
            .status()
            .unwrap();
        assert_eq!(status.code(), Some(42));
        tokio::runtime::Runtime::new().unwrap().block_on(async {
            let storage = store::Store::open(dir.0.clone()).await.unwrap();
            let version = storage.lookup("crash".into()).await.unwrap().unwrap();
            wait_ready(&storage, &version).await;
            assert_eq!(
                std::fs::read(dir.0.join(format!("{}.ok", version.meta.generation))).unwrap(),
                vec![6; 700000]
            );
            storage.shutdown().await.unwrap();
        });
    }
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
#[ignore = "requires ffmpeg executable"]
async fn published_image_is_decodable_as_an_ordinary_file() {
    let png = std::process::Command::new("ffmpeg")
        .args([
            "-nostdin",
            "-v",
            "error",
            "-f",
            "lavfi",
            "-i",
            "color=c=red:s=2x2:r=1",
            "-frames:v",
            "1",
            "-f",
            "image2pipe",
            "-vcodec",
            "png",
            "pipe:1",
        ])
        .output()
        .unwrap();
    assert!(
        png.status.success(),
        "{}",
        String::from_utf8_lossy(&png.stderr)
    );
    assert!(png.stdout.starts_with(b"\x89PNG"));
    let dir = Directory::new();
    let storage = store::Store::open(dir.0.clone()).await.unwrap();
    let version = storage
        .create(image_meta("png", png.stdout.len() as u64))
        .await
        .unwrap();
    write_all(&storage, &version, &png.stdout).await;
    wait_ready(&storage, &version).await;
    let file = storage
        .local_file(version.clone(), "client".into())
        .await
        .unwrap()
        .unwrap();
    let output = std::process::Command::new("ffmpeg")
        .args(["-nostdin", "-v", "error", "-i"])
        .arg(&file.path)
        .args(["-frames:v", "1", "-f", "null", "-"])
        .output()
        .unwrap();
    assert!(
        output.status.success(),
        "{}",
        String::from_utf8_lossy(&output.stderr)
    );
    assert_eq!(std::fs::read(&file.path).unwrap(), png.stdout);
    storage
        .release_file(file.token, "client".into())
        .await
        .unwrap();
    storage.shutdown().await.unwrap();
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn http_file_redirect_and_lease_round_trip() {
    let origin = Origin::new(500, true, false, Duration::ZERO).await;
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let mut warm = origin.request(None);
    warm.kind = 0;
    warm.headers.insert("accept", "*/*".parse().unwrap());
    body(cache.response(warm.clone()).await.unwrap()).await;
    tokio::time::timeout(Duration::from_secs(8), async {
        while !std::fs::read_dir(dir.0.join("resources-v1"))
            .unwrap()
            .any(|e| e.unwrap().path().extension().is_some_and(|s| s == "ok"))
        {
            tokio::time::sleep(Duration::from_millis(10)).await;
        }
    })
    .await
    .unwrap();
    let key = uuid::Uuid::new_v4().simple().to_string();
    cache.authorize_file_client(&key).unwrap();
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    let base = format!("http://{}", listener.local_addr().unwrap());
    let server_cache = cache.clone();
    let server = tokio::spawn(async move {
        let mut connections = tokio::task::JoinSet::new();
        while let Ok((socket, _)) = listener.accept().await {
            let cache = server_cache.clone();
            let warm = warm.clone();
            connections.spawn(async move {
                let service = hyper::service::service_fn(
                    move |request: hyper::Request<hyper::body::Incoming>| {
                        let cache = cache.clone();
                        let mut resource = warm.clone();
                        async move {
                            if let Some(response) = cache
                                .file_lease_response(
                                    request.method(),
                                    request.uri().path(),
                                    request.headers(),
                                )
                                .await
                            {
                                return Ok::<_, crate::error::ProcessError>(response);
                            }
                            resource.headers = request.headers().clone();
                            cache.response(resource).await
                        }
                    },
                );
                let _ = hyper::server::conn::http1::Builder::new()
                    .serve_connection(hyper_util::rt::TokioIo::new(socket), service)
                    .await;
            });
        }
    });
    let client = reqwest::Client::builder()
        .redirect(reqwest::redirect::Policy::none())
        .build()
        .unwrap();
    let response = client
        .get(format!("{base}/image"))
        .header("qcm-file-client", &key)
        .header("qextra-image-transport", "file-lease-v1")
        .send()
        .await
        .unwrap();
    assert_eq!(response.status(), 302);
    let path = reqwest::Url::parse(response.headers()["location"].to_str().unwrap())
        .unwrap()
        .to_file_path()
        .unwrap();
    let renew = response.headers()["qcm-file-renew"]
        .to_str()
        .unwrap()
        .to_owned();
    let release = response.headers()["qcm-file-release"]
        .to_str()
        .unwrap()
        .to_owned();
    assert!(response.bytes().await.unwrap().is_empty());
    assert_eq!(std::fs::read(&path).unwrap(), origin.expected(0, 500, 1));
    assert_eq!(
        client
            .post(format!("{base}{renew}"))
            .header("qcm-file-client", &key)
            .send()
            .await
            .unwrap()
            .status(),
        204
    );
    assert_eq!(
        client
            .delete(format!("{base}{release}"))
            .header("qcm-file-client", &key)
            .send()
            .await
            .unwrap()
            .status(),
        204
    );
    assert_eq!(
        client
            .post(format!("{base}{renew}"))
            .header("qcm-file-client", &key)
            .send()
            .await
            .unwrap()
            .status(),
        410
    );
    server.abort();
    let _ = server.await;
    cache.shutdown().await.unwrap();
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
#[ignore = "targeted file conversion space and latency measurement"]
async fn bounded_conversion_space_and_online_reads() {
    let dir = Directory::new();
    let storage = store::Store::open_blocks(dir.0.clone()).await.unwrap();
    let length = 64 * 1024 * 1024 + 17;
    let version = storage.create(meta("large", length)).await.unwrap();
    for block in (0..version.meta.geometry.count()).rev() {
        let range = version.meta.geometry.range(block).unwrap();
        storage
            .write(
                version.clone(),
                vec![(
                    block,
                    Bytes::from(vec![
                        (block % 251) as u8;
                        (range.end - range.start) as usize
                    ]),
                )],
            )
            .await
            .unwrap();
    }
    let before = storage.usage().await.unwrap().audio;
    let start = std::time::Instant::now();
    storage.test_materialization(true).await.unwrap();
    let mut peak = before;
    let mut max_read = Duration::ZERO;
    let mut samples = 0;
    while !storage.plain_ready(version.clone()).await.unwrap() {
        peak = peak.max(storage.usage().await.unwrap().audio);
        let began = std::time::Instant::now();
        assert_eq!(
            storage.read(version.clone(), 3).await.unwrap().unwrap()[0],
            3
        );
        max_read = max_read.max(began.elapsed());
        samples += 1;
        assert!(start.elapsed() < Duration::from_secs(30));
        tokio::task::yield_now().await;
    }
    let after = storage.usage().await.unwrap().audio;
    assert!(peak <= 2 * length + format::MAX_BLOCK + 8192);
    assert!(after <= length + 8192);
    assert_eq!(
        storage
            .read(version.clone(), version.meta.geometry.count() - 1)
            .await
            .unwrap()
            .unwrap()
            .len(),
        17
    );
    eprintln!("conversion bytes={length} before={before} peak={peak} after={after} elapsed_us={} read_samples={samples} max_read_us={}", start.elapsed().as_micros(), max_read.as_micros());
    storage.shutdown().await.unwrap();
}
