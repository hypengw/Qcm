use super::*;
use futures_util::FutureExt;
use qcm_core::{
    db::sync_run::SyncCoordinator,
    global,
    provider::{AuthInfo, AuthMethod, AuthResult, Context},
};
use qcm_plugin_lua::{plugin::Capabilities, provider::LuaProvider};
use sea_orm::{ConnectOptions, ConnectionTrait, Database};
use std::path::PathBuf;

#[tokio::test(flavor = "multi_thread", worker_threads = 4)]
#[ignore = "requires an authorized Jellyfin server and the silent playback-io test executable"]
async fn jellyfin_hls_session_cache_and_native_player() {
    const ID: i64 = 879371;
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../plugin/lua/jellyfin");
    let caps: Capabilities =
        serde_json::from_slice(&std::fs::read(root.join("plugin.json")).unwrap()).unwrap();
    let p = Arc::new(
        LuaProvider::with_capabilities(
            Some(ID),
            "HLS test",
            &format!("qcm-hls-{}", uuid::Uuid::new_v4()),
            "jellyfin",
            &root.join("src/provider.lua"),
            caps,
        )
        .unwrap(),
    );
    let mut options = ConnectOptions::new("sqlite::memory:");
    options.max_connections(1);
    let db = Database::connect(options).await.unwrap();
    db.execute_unprepared("CREATE TABLE song(id INTEGER PRIMARY KEY); CREATE TABLE item(id INTEGER PRIMARY KEY,native_id TEXT,provider_id INTEGER);").await.unwrap();
    let item = std::env::var("JELLYFIN_ITEM").unwrap();
    db.execute(sea_orm::Statement::from_sql_and_values(
        sea_orm::DatabaseBackend::Sqlite,
        "INSERT INTO item(id,native_id,provider_id) VALUES(1,?,?)",
        [item.clone().into(), ID.into()],
    ))
    .await
    .unwrap();
    db.execute_unprepared("INSERT INTO song VALUES(1)")
        .await
        .unwrap();
    let (ev_sender, mut receiver) = tokio::sync::mpsc::channel(8);
    let drain = tokio::spawn(async move { while receiver.recv().await.is_some() {} });
    let context = Arc::new(Context {
        db: db.clone(),
        cache_db: db,
        ev_sender,
        sync_coordinator: Arc::new(SyncCoordinator::default()),
    });
    let base = std::env::var("JELLYFIN_URL").unwrap();
    assert!(matches!(
        p.auth(
            &context,
            &AuthInfo {
                server_url: base.clone(),
                method: Some(AuthMethod::Username {
                    username: std::env::var("JELLYFIN_USER").unwrap(),
                    pw: std::env::var("JELLYFIN_PASSWORD").unwrap()
                })
            }
        )
        .await
        .unwrap(),
        AuthResult::Ok
    ));
    global::add_provider(p.clone());
    let registered = scopeguard::guard((), |_| global::remove_provider(ID));
    let directory = std::env::temp_dir().join(format!("qcm-hls-e2e-{}", uuid::Uuid::new_v4()));
    let _directory = scopeguard::guard(directory.clone(), |path| {
        let _ = std::fs::remove_dir_all(path);
    });
    let cache = crate::resource_cache::ResourceCache::open(directory.clone())
        .await
        .unwrap();
    let (oper, _) = crate::task::TaskManager::new();
    let (backend_ev, _) = tokio::sync::mpsc::channel(1);
    let service = Arc::new(ServiceContext {
        provider_context: context,
        backend_ev,
        oper,
        resource_cache: cache.clone(),
        radio: Arc::new(Default::default()),
        playback: Arc::new(Default::default()),
    });
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    let address = listener.local_addr().unwrap();
    let url = format!("http://{address}/playback/source/Song/1");
    let server = tokio::spawn({
        let service = service.clone();
        async move {
            let mut connections = tokio::task::JoinSet::new();
            while let Ok((socket, _)) = listener.accept().await {
                let service = service.clone();
                connections.spawn(async move {
                    let handler = hyper::service::service_fn(move |request| {
                        let service = service.clone();
                        async move {
                            if std::env::var_os("QCM_HLS_TRACE").is_some() {
                                eprintln!(
                                    "local request {} range={:?}",
                                    request.uri().path(),
                                    request.headers().get("range")
                                );
                            }
                            let response = match handle(&service, request).await {
                                Ok(response) => response,
                                Err(error) => {
                                    eprintln!("playback HTTP failure: {error}");
                                    Response::builder()
                                        .status(match error {
                                            ProcessError::NotFound => 404,
                                            ProcessError::NotAuth => 401,
                                            _ => 502,
                                        })
                                        .body(ResponseBody::Empty)
                                        .unwrap()
                                }
                            };
                            Ok::<_, std::convert::Infallible>(response)
                        }
                    });
                    let _ = hyper::server::conn::http1::Builder::new()
                        .serve_connection(hyper_util::rt::TokioIo::new(socket), handler)
                        .await;
                });
            }
        }
    });
    let saved: serde_json::Value = serde_json::from_str(&p.save()).unwrap();
    let token = saved["token"].as_str().unwrap().to_owned();
    let result = std::panic::AssertUnwindSafe(async {
        let executable = std::env::var("QCM_HLS_PLAYER").unwrap();
        let short = std::env::var_os("QCM_HLS_SHORT").is_some();
        let output = tokio::task::spawn_blocking(move || {
            let mut command = std::process::Command::new(executable);
            command.arg(url);
            if short {
                command.arg("short");
            }
            command.output().unwrap()
        })
        .await
        .unwrap();
        let diagnostic = String::from_utf8_lossy(&output.stderr).replace(&token, "<token>");
        assert!(
            output.status.success(),
            "player exit={:?}: {} {diagnostic}",
            output.status.code(),
            String::from_utf8_lossy(&output.stdout)
        );
        eprintln!("{}", String::from_utf8_lossy(&output.stdout));
        assert!(
            service.playback.sessions.lock().await.is_empty(),
            "native close retained a playback session"
        );
        let usage = cache.usage().await.unwrap();
        assert!(usage.audio > 0 && usage.audio < 32 * 1024 * 1024);
        let packs = std::fs::read_dir(directory.join("resources-v1"))
            .unwrap()
            .filter_map(Result::ok)
            .filter(|e| e.path().extension().is_some_and(|s| s == "pack"))
            .count();
        assert!((1..=4).contains(&packs));
        eprintln!("audio_bytes={} pack_files={packs}", usage.audio);

        let client = reqwest::Client::new();
        let endpoint = format!("http://{}/playback/source/Song/1", address);
        let first: serde_json::Value = client
            .post(&endpoint)
            .json(&json!({"max_bitrate":128000}))
            .send()
            .await
            .unwrap()
            .error_for_status()
            .unwrap()
            .json()
            .await
            .unwrap();
        let second: serde_json::Value = client
            .post(&endpoint)
            .json(&json!({"max_bitrate":192000}))
            .send()
            .await
            .unwrap()
            .error_for_status()
            .unwrap()
            .json()
            .await
            .unwrap();
        let first_id = first["session"].as_str().unwrap();
        let second_id = second["session"].as_str().unwrap();
        assert_ne!(first_id, second_id);
        let base = format!("http://{address}");
        assert_eq!(
            client
                .post(format!("{base}/playback/{first_id}/close"))
                .send()
                .await
                .unwrap()
                .status(),
            200
        );
        assert_eq!(
            client
                .get(format!("{base}{}", first["url"].as_str().unwrap()))
                .send()
                .await
                .unwrap()
                .status(),
            404
        );
        assert_eq!(
            client
                .head(format!("{base}{}", second["url"].as_str().unwrap()))
                .send()
                .await
                .unwrap()
                .status(),
            200
        );
        service.playback.invalidate(ID).await;
        assert_eq!(
            client
                .get(format!("{base}{}", second["url"].as_str().unwrap()))
                .send()
                .await
                .unwrap()
                .status(),
            404
        );
        assert_eq!(
            client
                .post(format!("{base}/playback/{first_id}/close"))
                .send()
                .await
                .unwrap()
                .status(),
            200
        );
        service.playback.shutdown().await;
        assert_eq!(service.playback.slots.available_permits(),MAX_SESSIONS);
    })
    .catch_unwind()
    .await;
    service.playback.shutdown().await;
    cache.shutdown().await.unwrap();
    server.abort();
    let _ = server.await;
    drain.abort();
    let logout = reqwest::Client::new()
        .post(format!("{base}/Sessions/Logout"))
        .header("x-emby-token", &token)
        .send()
        .await
        .unwrap();
    assert_eq!(logout.status(), 204);
    drop(registered);
    if let Err(panic) = result {
        std::panic::resume_unwind(panic);
    }
}
