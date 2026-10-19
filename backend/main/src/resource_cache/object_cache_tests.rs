use super::*;
fn object_request(origin: &Origin, range: Option<&str>) -> ResourceRequest {
    let mut request = origin.request(range);
    request.prepared = Arc::new(
        Arc::try_unwrap(request.prepared)
            .ok()
            .unwrap()
            .with_cache_mode(qcm_core::resource::CacheMode::WholeObject),
    );
    request
}
#[tokio::test]
async fn objects_use_complete_responses_for_weak_ranges_and_revalidate() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(102648, true, true, Duration::ZERO).await;
    let response = cache
        .response(object_request(&origin, Some("bytes=100-611")))
        .await
        .unwrap();
    assert_eq!(response.status(), 206);
    assert_eq!(response.headers()["content-range"], "bytes 100-611/102648");
    assert_eq!(body(response).await, origin.expected(100, 612, 1));
    assert!(!origin.hits()[0].contains_key("range"));
    assert_eq!(
        body(
            cache
                .response(object_request(&origin, Some("bytes=-32")))
                .await
                .unwrap()
        )
        .await,
        origin.expected(102616, 102648, 1)
    );
    assert_eq!(origin.hits().len(), 1);
    let mut request = object_request(&origin, None);
    request
        .headers
        .insert("cache-control", "no-cache".parse().unwrap());
    assert_eq!(
        body(cache.response(request).await.unwrap()).await,
        origin.expected(0, 102648, 1)
    );
    assert_eq!(origin.hits().len(), 2);
    assert!(origin.hits()[1].contains_key("if-none-match"));
    let invalid = cache
        .response(object_request(&origin, Some("bytes=999999-")))
        .await
        .unwrap();
    assert_eq!(invalid.status(), 416);
    drop(invalid);
    cache.shutdown().await.unwrap();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    assert_eq!(
        body(cache.response(object_request(&origin, None)).await.unwrap()).await,
        origin.expected(0, 102648, 1)
    );
    assert_eq!(origin.hits().len(), 2);
    cache.shutdown().await.unwrap();
}
#[tokio::test]
async fn objects_reject_truncation_and_account_invalidation() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(102648, true, false, Duration::ZERO).await;
    origin.state.truncate.store(true, Ordering::SeqCst);
    assert!(cache.response(object_request(&origin, None)).await.is_err());
    origin.state.truncate.store(false, Ordering::SeqCst);
    let response = cache.response(object_request(&origin, None)).await.unwrap();
    assert_eq!(body(response).await, origin.expected(0, 102648, 1));
    cache.invalidate(1).await.unwrap();
    assert_eq!(cache.usage().await.unwrap().audio, 0);
    cache.shutdown().await.unwrap();
}
#[tokio::test]
async fn objects_unknown_length_no_store_and_failed_storage_remain_bounded() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(102648, false, false, Duration::ZERO).await;
    origin.state.unknown.store(true, Ordering::SeqCst);
    assert_eq!(
        body(cache.response(object_request(&origin, None)).await.unwrap()).await,
        origin.expected(0, 102648, 1)
    );
    assert_eq!(
        body(cache.response(object_request(&origin, None)).await.unwrap()).await,
        origin.expected(0, 102648, 1)
    );
    assert_eq!(origin.hits().len(), 1);
    cache.invalidate(1).await.unwrap();
    *origin.state.control.lock().unwrap() = "no-store".into();
    assert_eq!(
        body(cache.response(object_request(&origin, None)).await.unwrap()).await,
        origin.expected(0, 102648, 1)
    );
    assert_eq!(cache.usage().await.unwrap().audio, 0);
    *origin.state.control.lock().unwrap() = "max-age=3600".into();
    cache
        .store
        .as_ref()
        .unwrap()
        .fault(store::Fault::Sql)
        .await
        .unwrap();
    assert_eq!(
        body(cache.response(object_request(&origin, None)).await.unwrap()).await,
        origin.expected(0, 102648, 1)
    );
    assert_eq!(
        body(cache.response(object_request(&origin, None)).await.unwrap()).await,
        origin.expected(0, 102648, 1)
    );
    assert_eq!(origin.hits().len(), 4);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn objects_cancel_stalled_body_without_publishing() {
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let origin = Origin::new(256 * 1024, true, false, Duration::from_secs(10)).await;
    let lifetime = Arc::new(qcm_core::resource::ResourceLifetime::default());
    let mut request = object_request(&origin, None);
    request.prepared = Arc::new(
        Arc::try_unwrap(request.prepared)
            .ok()
            .unwrap()
            .with_lifetime(lifetime.clone()),
    );
    let active = cache.clone();
    let task = tokio::spawn(async move { active.response(request).await });
    tokio::time::timeout(Duration::from_secs(2), async {
        while origin.hits().is_empty() {
            tokio::task::yield_now().await;
        }
    })
    .await
    .unwrap();
    lifetime.cancel();
    assert!(tokio::time::timeout(Duration::from_secs(1), task)
        .await
        .unwrap()
        .unwrap()
        .is_err());
    assert_eq!(cache.usage().await.unwrap().audio, 0);
    cache.shutdown().await.unwrap();
}

#[tokio::test]
async fn objects_over_admission_limit_preserve_reader_ranges() {
    for unknown in [false, true] {
        let directory = Directory::new();
        let cache = ResourceCache::open(directory.0.clone()).await.unwrap();
        let origin = Origin::new(
            store::MAX_OBJECT as usize + 1024,
            true,
            false,
            Duration::ZERO,
        )
        .await;
        origin.state.unknown.store(unknown, Ordering::SeqCst);
        let response = cache
            .response(object_request(&origin, Some("bytes=100-299")))
            .await
            .unwrap();
        assert_eq!(response.status(), 206);
        assert_eq!(body(response).await, origin.expected(100, 300, 1));
        assert_eq!(origin.hits().len(), 2);
        assert!(!origin.hits()[0].contains_key("range"));
        assert_eq!(origin.hits()[1]["range"], "bytes=100-299");
        assert_eq!(cache.usage().await.unwrap().audio, 0);
        cache.shutdown().await.unwrap();
    }
}
