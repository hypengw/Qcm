use super::*;

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
#[ignore = "requires ffmpeg executable"]
async fn ffmpeg_seeks_through_resource_cache_http() {
    use hyper_util::rt::TokioIo;
    let mut wav = Vec::new();
    let data_length = 48000u32 * 16 * 2;
    wav.extend_from_slice(b"RIFF");
    wav.extend_from_slice(&(36 + data_length).to_le_bytes());
    wav.extend_from_slice(b"WAVEfmt ");
    wav.extend_from_slice(&16u32.to_le_bytes());
    wav.extend_from_slice(&1u16.to_le_bytes());
    wav.extend_from_slice(&1u16.to_le_bytes());
    wav.extend_from_slice(&48000u32.to_le_bytes());
    wav.extend_from_slice(&96000u32.to_le_bytes());
    wav.extend_from_slice(&2u16.to_le_bytes());
    wav.extend_from_slice(&16u16.to_le_bytes());
    wav.extend_from_slice(b"data");
    wav.extend_from_slice(&data_length.to_le_bytes());
    wav.resize(44 + data_length as usize, 0);
    let origin = Origin::new(wav.len(), true, false, Duration::ZERO).await;
    *origin.state.payload.lock().unwrap() = Some(Arc::new(wav));
    let dir = Directory::new();
    let cache = ResourceCache::open(dir.0.clone()).await.unwrap();
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    let url = format!("http://{}/audio.wav", listener.local_addr().unwrap());
    let prepared = origin.request(None).prepared;
    body(
        cache
            .response(ResourceRequest {
                provider: 1,
                kind: 1,
                scope: "ffmpeg".into(),
                prepared: prepared.clone(),
                headers: HeaderMap::new(),
            })
            .await
            .unwrap(),
    )
    .await;
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
    let server_cache = cache.clone();
    let ranges = Arc::new(Mutex::new(Vec::new()));
    let observed = ranges.clone();
    let server = tokio::spawn(async move {
        let mut connections = tokio::task::JoinSet::new();
        while let Ok((socket, _)) = listener.accept().await {
            let prepared = prepared.clone();
            let cache = server_cache.clone();
            let ranges = observed.clone();
            connections.spawn(async move {
                let service = hyper::service::service_fn(
                    move |incoming: hyper::Request<hyper::body::Incoming>| {
                        ranges.lock().unwrap().push(
                            incoming
                                .headers()
                                .get("range")
                                .and_then(|v| v.to_str().ok())
                                .unwrap_or("")
                                .to_string(),
                        );
                        let prepared = prepared.clone();
                        let cache = cache.clone();
                        async move {
                            cache
                                .response(ResourceRequest {
                                    provider: 1,
                                    kind: 1,
                                    scope: "ffmpeg".into(),
                                    prepared,
                                    headers: incoming.headers().clone(),
                                })
                                .await
                        }
                    },
                );
                let _ = hyper::server::conn::http1::Builder::new()
                    .serve_connection(TokioIo::new(socket), service)
                    .await;
            });
        }
    });
    let output = tokio::task::spawn_blocking(move || {
        std::process::Command::new("ffmpeg")
            .args([
                "-nostdin",
                "-v",
                "error",
                "-rw_timeout",
                "5000000",
                "-seekable",
                "1",
                "-short_seek_size",
                "0",
                "-ss",
                "3.5",
                "-i",
                &url,
                "-t",
                "0.1",
                "-f",
                "null",
                "-",
            ])
            .output()
            .unwrap()
    })
    .await
    .unwrap();
    server.abort();
    let _ = server.await;
    cache.shutdown().await.unwrap();
    assert!(
        output.status.success(),
        "{}",
        String::from_utf8_lossy(&output.stderr)
    );
    let ranges = ranges.lock().unwrap();
    assert!(
        ranges
            .iter()
            .any(|v| v.starts_with("bytes=") && !v.starts_with("bytes=0-")),
        "{ranges:?}"
    );
    eprintln!("ffmpeg seek smoke passed: {ranges:?}");
}
