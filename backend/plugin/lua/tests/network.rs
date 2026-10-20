use qcm_core::{
    network::{self, NetworkSettings, ProxyEntry},
    provider::{settings::SettingsSnapshot, Context, Provider, ProviderCommon, ProviderSession},
    resource::Resource,
};
use qcm_plugin_lua::provider::LuaProvider;
use std::sync::{Arc, Mutex};
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::TcpListener,
};

async fn server(
    label: &'static str,
    seen: Arc<Mutex<Vec<(String, String)>>>,
) -> (String, tokio::task::JoinHandle<()>) {
    let listener = TcpListener::bind("127.0.0.1:0").await.unwrap();
    let url = format!("http://{}", listener.local_addr().unwrap());
    let task = tokio::spawn(async move {
        let mut tasks = tokio::task::JoinSet::new();
        loop {
            let (mut socket, _) = listener.accept().await.unwrap();
            let seen = seen.clone();
            tasks.spawn(async move {
                loop {
                    let mut bytes = Vec::new();
                    loop {
                        let mut byte = [0];
                        if socket.read(&mut byte).await.unwrap() == 0 { return; }
                        bytes.push(byte[0]);
                        if bytes.ends_with(b"\r\n\r\n") { break; }
                    }
                    seen.lock().unwrap().push((label.into(), String::from_utf8(bytes).unwrap()));
                    let response = format!("HTTP/1.1 200 OK\r\nContent-Length: {}\r\nSet-Cookie: route=kept; Path=/\r\n\r\n{label}", label.len());
                    socket.write_all(response.as_bytes()).await.unwrap();
                }
            });
        }
    });
    (url, task)
}
fn expect(provider: &LuaProvider, value: &str) {
    provider.set_settings(SettingsSnapshot {
        revision: 0,
        values: serde_json::from_value(serde_json::json!({"expected":value})).unwrap(),
    });
}
fn route(url: Option<&str>) {
    network::publish(
        network::compile(NetworkSettings {
            revision: 0,
            proxies: url
                .map(|url| {
                    vec![ProxyEntry {
                        name: "Route".into(),
                        url: url.into(),
                        provider_ids: vec![90101],
                        ..Default::default()
                    }]
                })
                .unwrap_or_default(),
        })
        .unwrap(),
    );
}
#[tokio::test]
async fn provider_network_routes_cached_transport_batches_and_prepared_resources() {
    let path = std::env::temp_dir().join(format!("qcm-network-{}.lua", uuid::Uuid::new_v4()));
    std::fs::write(
        &path,
        r#"
local M, http, retired = {}, nil, nil
function M.load() retired=http; http=qcm.get_http_client() end
function M.save() return '' end
function M.login() return {type='Ok'} end
function M.check()
    if qcm.inner:settings().expected=='retired' then
        assert(not pcall(function() retired:get(qcm.inner.base_url..'/stale') end))
        return
    end
    local response=http:send {method='GET',url=qcm.inner.base_url..'/api'}
    assert(response:text()==qcm.inner:settings().expected)
    local batch=http:new_batch()
    batch:add(http:get(qcm.inner.base_url..'/batch'))
    batch:seal()
    assert(batch:wait_one()==qcm.inner:settings().expected)
    batch:close()
end
function M.prepare_audio()
    return http:request('GET',qcm.inner.base_url..'/audio'):stream {headers_timeout_ms=1000}:build()
end
function M.sync() end
function M.sync_item() end
function M.favorite() end
function M.image() end
function M.audio() end
function M.subtitle() end
return M
"#,
    )
    .unwrap();
    let seen = Arc::new(Mutex::new(Vec::new()));
    let (origin, origin_task) = server("direct", seen.clone()).await;
    let (first, first_task) = server("proxy-a", seen.clone()).await;
    let (second, second_task) = server("proxy-b", seen.clone()).await;
    let provider = LuaProvider::new(Some(90101), "Network", "test", "test", &path).unwrap();
    provider.load_auth_info(&origin, None);
    provider.load("");
    expect(&provider, "direct");
    let other = LuaProvider::new(Some(90102), "Other", "test", "test", &path).unwrap();
    other.load_auth_info(&origin, None);
    other.load("");
    expect(&other, "direct");
    let db = sea_orm::Database::connect("sqlite::memory:").await.unwrap();
    let (events, _) = tokio::sync::mpsc::channel(1);
    let ctx = Context {
        db: db.clone(),
        cache_db: db,
        ev_sender: events,
        sync_coordinator: Default::default(),
    };
    provider.check(&ctx).await.unwrap();
    let prepared = provider
        .prepare_resource(Resource::Audio {
            item_id: "audio".into(),
        })
        .await
        .unwrap();
    let epoch = provider.lifecycle_epoch();
    let key = provider.session_key();
    for (url, label) in [(&first, "proxy-a"), (&second, "proxy-b")] {
        route(Some(url));
        expect(&provider, label);
        provider.check(&ctx).await.unwrap();
        assert_eq!(
            prepared
                .fetch(Default::default())
                .await
                .unwrap()
                .text()
                .await
                .unwrap(),
            label
        );
        other.check(&ctx).await.unwrap();
    }
    let temporary = LuaProvider::new(None, "Temporary", "test", "test", &path).unwrap();
    temporary.set_network_scope(Some(90101));
    temporary.load_auth_info(&origin, None);
    temporary.load("");
    expect(&temporary, "proxy-b");
    temporary.check(&ctx).await.unwrap();
    assert_eq!(temporary.id(), None);
    route(None);
    expect(&provider, "direct");
    provider.check(&ctx).await.unwrap();
    assert_eq!(
        prepared
            .fetch(Default::default())
            .await
            .unwrap()
            .text()
            .await
            .unwrap(),
        "direct"
    );
    assert_eq!(provider.lifecycle_epoch(), epoch);
    assert_eq!(provider.session_key(), key);
    assert!(provider.save_cookie().contains("kept"));
    assert!(prepared.is_valid());
    let seen = seen.lock().unwrap();
    assert_eq!(
        seen.iter().filter(|(label, _)| label == "proxy-a").count(),
        3
    );
    assert_eq!(
        seen.iter().filter(|(label, _)| label == "proxy-b").count(),
        5
    );
    assert_eq!(
        seen.iter().filter(|(label, _)| label == "direct").count(),
        9
    );
    for (_, request) in seen.iter().filter(|(label, _)| label == "proxy-a") {
        assert!(request.starts_with("GET http://"));
        assert!(request.to_ascii_lowercase().contains("cookie: route=kept"));
    }
    drop(seen);
    provider.load("");
    expect(&provider, "retired");
    provider.check(&ctx).await.unwrap();
    assert!(!prepared.is_valid());
    provider.invalidate();
    origin_task.abort();
    first_task.abort();
    second_task.abort();
    std::fs::remove_file(path).unwrap();
}
