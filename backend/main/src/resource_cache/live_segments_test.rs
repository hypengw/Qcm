use super::*;
use futures_util::{FutureExt, TryStreamExt};
use hyper::{body::Frame, Response};
use hyper_util::rt::TokioIo;
use std::sync::atomic::AtomicU64;

#[tokio::test(flavor = "multi_thread", worker_threads = 4)]
#[ignore = "requires authorized Jellyfin credentials and optional QCM_RESOURCE_PLAYER executable"]
async fn jellyfin_segmented_seek_and_player() {
    let base = std::env::var("QCM_JELLYFIN_URL").unwrap();
    let item = std::env::var("QCM_JELLYFIN_ITEM").unwrap();
    let client = reqwest::Client::builder()
        .redirect(reqwest::redirect::Policy::none())
        .build()
        .unwrap();
    let auth: serde_json::Value = client.post(format!("{base}/Users/AuthenticateByName"))
        .header("authorization",format!("MediaBrowser Client=\"Qcm segment test\", Device=\"silent decoder\", DeviceId=\"{}\", Version=\"1\"",uuid::Uuid::new_v4()))
        .json(&serde_json::json!({"Username":std::env::var("QCM_JELLYFIN_USER").unwrap(),"Pw":std::env::var("QCM_JELLYFIN_PASSWORD").unwrap()}))
        .send().await.unwrap().error_for_status().unwrap().json().await.unwrap();
    let token = auth["AccessToken"].as_str().unwrap().to_string();
    let source = format!("{base}/Audio/{item}/stream?static=true");
    let source_listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    let local_source = format!("http://{}/stream", source_listener.local_addr().unwrap());
    let bytes = Arc::new(AtomicU64::new(0));
    let hits = Arc::new(Mutex::new(Vec::new()));
    let shared_bytes = bytes.clone();
    let shared_hits = hits.clone();
    let proxy_client = client.clone();
    let proxy_token = token.clone();
    let proxy_source = source.clone();
    let proxy = tokio::spawn(async move {
        let mut connections = tokio::task::JoinSet::new();
        while let Ok((socket, _)) = source_listener.accept().await {
            let client = proxy_client.clone();
            let token = proxy_token.clone();
            let source = proxy_source.clone();
            let bytes = shared_bytes.clone();
            let hits = shared_hits.clone();
            connections.spawn(async move {
                let service = hyper::service::service_fn(move |incoming: hyper::Request<hyper::body::Incoming>| {
                    let client = client.clone(); let token = token.clone(); let source = source.clone();
                    let bytes = bytes.clone(); let hits = hits.clone();
                    async move {
                        let mut request = client.get(source).header("x-emby-token",token).header("accept-encoding","identity");
                        for key in ["range","if-range","if-modified-since","if-none-match"] {
                            if let Some(v) = incoming.headers().get(key) { request = request.header(key,v); }
                        }
                        let upstream = request.send().await.map_err(|e| e.without_url())?;
                        let status = upstream.status(); let headers = policy::forwarded(upstream.headers());
                        hits.lock().unwrap().push(serde_json::json!({"range":policy::value(incoming.headers(),"range"),"if_range":policy::value(incoming.headers(),"if-range"),"status":status.as_u16(),"content_range":policy::value(&headers,"content-range")}));
                        let body = upstream.bytes_stream().map_ok(move |chunk| { bytes.fetch_add(chunk.len() as u64,Ordering::SeqCst); Frame::data(chunk) });
                        let mut response = Response::new(http_body_util::StreamBody::new(body));
                        *response.status_mut() = status; *response.headers_mut() = headers;
                        Ok::<_,reqwest::Error>(response)
                    }
                });
                let _ = hyper::server::conn::http1::Builder::new().serve_connection(TokioIo::new(socket),service).await;
            });
        }
    });
    let result = std::panic::AssertUnwindSafe(async {
        let dir = Directory::new();
        let mut cache = ResourceCache::open(dir.0.clone()).await.unwrap();
        let prepared = Arc::new(qcm_core::resource::PreparedResource::new(client.clone(),client.get(local_source).build().unwrap(),Duration::from_secs(10)).unwrap());
        let request = |start:u64,end:u64| {
            let mut headers = HeaderMap::new(); headers.insert("range",format!("bytes={start}-{end}").parse().unwrap());
            ResourceRequest { provider:1,kind:1,scope:"live-segments".into(),prepared:prepared.clone(),headers }
        };
        let mut measurements = Vec::new();
        for (name,start,end) in [("cold_middle",58459896,58460919),("cross_segment",16777093,16778092),("cold_tail",116918771,116919794),("repeat_middle",58459896,58460919)] {
            let before = bytes.load(Ordering::SeqCst); let started = std::time::Instant::now();
            let response = cache.response(request(start,end)).await.unwrap();
            let headers_us = started.elapsed().as_micros();
            assert_eq!(response.status(),206);
            assert_eq!(response.headers()["content-range"],format!("bytes {start}-{end}/116919795"));
            let actual = body(response).await;
            let expected = client.get(&source).header("x-emby-token",&token).header("range",format!("bytes={start}-{end}")).send().await.unwrap().bytes().await.unwrap();
            assert!(actual == expected);
            tokio::time::sleep(Duration::from_millis(100)).await;
            let transferred = bytes.load(Ordering::SeqCst)-before;
            if name == "repeat_middle" { assert_eq!(transferred,0); }
            measurements.push(serde_json::json!({"name":name,"headers_us":headers_us,"upstream_bytes":transferred}));
        }
        cache.shutdown().await.unwrap();
        cache = ResourceCache::open(dir.0.clone()).await.unwrap();
        let before = bytes.load(Ordering::SeqCst);
        assert_eq!(body(cache.response(request(58459896,58460919)).await.unwrap()).await.len(),1024);
        assert_eq!(bytes.load(Ordering::SeqCst),before);
        if let Ok(player) = std::env::var("QCM_RESOURCE_PLAYER") {
            let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
            let url = format!("http://{}/audio/test",listener.local_addr().unwrap());
            let server_cache = cache.clone(); let prepared = prepared.clone();
            let server = tokio::spawn(async move {
                let mut connections = tokio::task::JoinSet::new();
                while let Ok((socket,_)) = listener.accept().await {
                    let cache = server_cache.clone(); let prepared = prepared.clone();
                    connections.spawn(async move {
                        let service = hyper::service::service_fn(move |req:hyper::Request<hyper::body::Incoming>| {
                            let cache = cache.clone(); let prepared = prepared.clone();
                            async move { cache.response(ResourceRequest {provider:1,kind:1,scope:"live-segments".into(),prepared,headers:req.headers().clone()}).await }
                        });
                        let _ = hyper::server::conn::http1::Builder::new().serve_connection(TokioIo::new(socket),service).await;
                    });
                }
            });
            let output = tokio::task::spawn_blocking(move || std::process::Command::new(player).arg(url).output().unwrap()).await.unwrap();
            server.abort(); let _ = server.await;
            assert!(output.status.success(),"player status={:?} stderr={}",output.status.code(),String::from_utf8_lossy(&output.stderr));
            eprintln!("{}",String::from_utf8_lossy(&output.stdout));
        }
        let usage = cache.usage().await.unwrap();
        let files = std::fs::read_dir(dir.0.join("resources-v1")).unwrap().filter(|e|e.as_ref().unwrap().path().extension().is_some_and(|v|v=="seg")).count();
        cache.shutdown().await.unwrap();
        eprintln!("live_segment_metrics={}",serde_json::json!({"requests":hits.lock().unwrap().clone(),"measurements":measurements,"total_upstream_bytes":bytes.load(Ordering::SeqCst),"disk_audio":usage.audio,"disk_metadata":usage.metadata,"segment_files":files}));
        assert!(bytes.load(Ordering::SeqCst) < 32*1024*1024);
    }).catch_unwind().await;
    proxy.abort();
    let _ = proxy.await;
    assert_eq!(
        client
            .post(format!("{base}/Sessions/Logout"))
            .header("x-emby-token", token)
            .send()
            .await
            .unwrap()
            .status(),
        204
    );
    result.unwrap();
}
