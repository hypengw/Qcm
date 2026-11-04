use migration::{MigrationTrait, SchemaManager};
use qcm_core::{
    db::sync_run::SyncCoordinator,
    plugin::{PluginContext, MAX_STATE_BYTES},
    provider::{Context, Provider},
};
use qcm_plugin_lua::{
    plugin::{PluginConfig, PluginManifest, API_VERSION},
    provider::LuaProvider,
};
use sea_orm::{ConnectionTrait, Database, DatabaseConnection};
use std::{path::PathBuf, sync::Arc};

struct Directory(PathBuf);

impl Directory {
    fn new() -> Self {
        let path = std::env::temp_dir().join(format!("qcm-installation-{}", uuid::Uuid::new_v4()));
        std::fs::create_dir(&path).unwrap();
        Self(path)
    }
}

impl Drop for Directory {
    fn drop(&mut self) {
        let _ = std::fs::remove_dir_all(&self.0);
    }
}

async fn open(url: &str) -> DatabaseConnection {
    let mut options = sea_orm::ConnectOptions::new(url);
    options
        .max_connections(if url == "sqlite::memory:" { 1 } else { 2 })
        .sqlx_logging(false);
    let db = Database::connect(options).await.unwrap();
    db.execute_unprepared("PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000")
        .await
        .unwrap();
    db
}

async fn migrate(db: &DatabaseConnection) {
    migration::m20261104_000001_plugin_installation_state::Migration
        .up(&SchemaManager::new(db))
        .await
        .unwrap();
}

fn installation(db: &DatabaseConnection, namespace: &str) -> PluginContext {
    PluginContext::new(namespace, "existing-installation-seed", db.clone()).unwrap()
}

fn call_context(db: &DatabaseConnection) -> Context {
    let (ev_sender, _) = tokio::sync::mpsc::channel(1);
    Context {
        db: db.clone(),
        cache_db: db.clone(),
        ev_sender,
        sync_coordinator: Arc::new(SyncCoordinator::default()),
    }
}

fn script(directory: &Directory, check: &str) -> PathBuf {
    let path = directory.0.join(format!("{}.lua", uuid::Uuid::new_v4()));
    std::fs::write(
        &path,
        format!(
            r#"
local p={{}}
local account='account-custom'
function p.save() return account end
function p.load(s) account=s end
function p.login() return {{type='Ok'}} end
function p.sync() end
function p.sync_item() end
function p.favorite() return true end
function p.image() end
function p.audio() end
function p.subtitle() end
function p.check() {check} end
return p
"#
        ),
    )
    .unwrap();
    path
}

#[tokio::test]
async fn installation_state_cas_is_shared_atomic_and_persistent() {
    let directory = Directory::new();
    let url = format!(
        "sqlite://{}?mode=rwc",
        directory.0.join("installation.db").display()
    );
    let first_db = open(&url).await;
    migrate(&first_db).await;
    let second_db = open(&url).await;
    let first = installation(&first_db, "synthetic");
    let second = installation(&second_db, "synthetic");
    assert_eq!(first.installation_id(), "existing-installation-seed");
    assert_eq!(first.runtime_id(), second.runtime_id());
    assert_ne!(first.runtime_id(), first.installation_id());
    assert_eq!(first.load_state().await.unwrap().revision, 0);
    assert!(first.load_state().await.unwrap().data.is_none());

    let barrier = Arc::new(tokio::sync::Barrier::new(3));
    let mut tasks = Vec::new();
    for (context, data) in [(first.clone(), "first"), (second.clone(), "second")] {
        let barrier = barrier.clone();
        tasks.push(tokio::spawn(async move {
            barrier.wait().await;
            context
                .compare_exchange_state(0, data.into())
                .await
                .unwrap()
        }));
    }
    barrier.wait().await;
    let a = tasks.remove(0).await.unwrap();
    let b = tasks.remove(0).await.unwrap();
    assert_ne!(a.applied, b.applied);
    assert_eq!(a.state, b.state);
    assert_eq!(a.state.revision, 1);
    assert_eq!(second.load_state().await.unwrap(), a.state);
    assert_eq!(
        installation(&first_db, "other")
            .load_state()
            .await
            .unwrap()
            .revision,
        0
    );

    let data = "opaque UTF-8: 中文\n\0not-json".to_owned();
    let updated = second
        .compare_exchange_state(1, data.clone())
        .await
        .unwrap();
    assert!(updated.applied);
    assert_eq!(updated.state.revision, 2);
    assert_eq!(updated.state.data.as_ref(), Some(&data));
    let stale = first
        .compare_exchange_state(1, "stale".into())
        .await
        .unwrap();
    assert!(!stale.applied);
    assert_eq!(stale.state, updated.state);
    first_db.execute_unprepared("CREATE TRIGGER reject_installation BEFORE UPDATE ON plugin_installation_state WHEN NEW.data='rejected' BEGIN SELECT RAISE(ABORT,'synthetic state failure'); END").await.unwrap();
    assert!(first
        .compare_exchange_state(2, "rejected".into())
        .await
        .is_err());
    assert_eq!(first.load_state().await.unwrap(), updated.state);

    drop((first, second));
    first_db.close().await.unwrap();
    second_db.close().await.unwrap();
    let reopened = open(&url).await;
    assert_eq!(
        installation(&reopened, "synthetic")
            .load_state()
            .await
            .unwrap(),
        updated.state
    );
    let other_installation = open("sqlite::memory:").await;
    migrate(&other_installation).await;
    assert_eq!(
        installation(&other_installation, "synthetic")
            .load_state()
            .await
            .unwrap()
            .revision,
        0
    );
    reopened.close().await.unwrap();
    other_installation.close().await.unwrap();
}

#[tokio::test]
async fn installation_state_rejects_invalid_inputs_and_reports_storage_failure() {
    let db = open("sqlite::memory:").await;
    migrate(&db).await;
    assert!(PluginContext::new("../other", "seed", db.clone()).is_err());
    assert!(PluginContext::new("", "seed", db.clone()).is_err());
    assert!(PluginContext::new("test", "", db.clone()).is_err());
    let context = installation(&db, "synthetic");
    assert!(context
        .compare_exchange_state(u64::MAX, "x".into())
        .await
        .is_err());
    assert!(context
        .compare_exchange_state(0, "x".repeat(MAX_STATE_BYTES + 1))
        .await
        .is_err());
    assert_eq!(context.load_state().await.unwrap().revision, 0);
    let empty = context
        .compare_exchange_state(0, String::new())
        .await
        .unwrap();
    assert_eq!(empty.state.data.as_deref(), Some(""));
    db.execute_unprepared("DROP TABLE plugin_installation_state")
        .await
        .unwrap();
    assert!(context.load_state().await.is_err());
    assert!(context.compare_exchange_state(1, "x".into()).await.is_err());
}

#[tokio::test]
async fn lua_installation_is_namespaced_and_survives_provider_session_replacement() {
    let directory = Directory::new();
    let db = open("sqlite::memory:").await;
    migrate(&db).await;
    let context = installation(&db, "synthetic");
    let first_path = script(
        &directory,
        r#"
local i=qcm.installation
assert(i.namespace=='synthetic' and i.installation_id=='existing-installation-seed')
assert(i.platform.source=='rust_target' and #i.platform.os>0 and #i.platform.arch>0)
assert(i.platform.android_id==nil and i.platform.imei==nil and i.platform.oaid==nil)
assert(not pcall(function() i.installation_id='replacement' end))
local platform=i.platform; platform.os='replacement'; assert(i.platform.os~='replacement')
local before=i:load_state(); assert(before.revision==0 and before.data==nil)
for _,revision in ipairs({-1,1.5,'0'}) do
    assert(not pcall(function() i:compare_exchange_state(revision,'invalid') end))
end
assert(not pcall(function() i:compare_exchange_state(0,{}) end))
assert(not pcall(function() i:compare_exchange_state(0,string.char(255)) end))
local result=i:compare_exchange_state(0,'installation-value')
assert(result.applied and result.state.revision==1)
qcm.inner:begin_session()
local after=i:load_state(); assert(after.revision==1 and after.data=='installation-value')
account='first-account'
"#,
    );
    let first = LuaProvider::with_context(
        Some(1),
        "first",
        "legacy-seed",
        "synthetic",
        &first_path,
        PluginConfig::default(),
        context.clone(),
    )
    .unwrap();
    first.check(&call_context(&db)).await.unwrap();
    assert_eq!(first.save(), "first-account");
    let second_path = script(
        &directory,
        r#"
local state=qcm.installation:load_state()
assert(state.revision==1 and state.data=='installation-value')
local conflict=qcm.installation:compare_exchange_state(0,'overwrite')
assert(not conflict.applied and conflict.state.data=='installation-value')
assert(account=='second-account')
"#,
    );
    let second = LuaProvider::with_context(
        None,
        "temporary",
        "legacy-seed",
        "synthetic",
        &second_path,
        PluginConfig::default(),
        context,
    )
    .unwrap();
    second.load("second-account");
    second.check(&call_context(&db)).await.unwrap();
    assert_eq!(first.save(), "first-account");
    assert_eq!(second.save(), "second-account");
}

#[tokio::test]
async fn current_api_exposes_context_response_and_url() {
    use std::io::{Read, Write};
    let directory = Directory::new();
    let path = script(&directory, "assert(qcm.installation==nil)");
    let db = open("sqlite::memory:").await;
    for api_version in [0, API_VERSION + 1, u32::MAX] {
        let unsupported = LuaProvider::with_config(
            None,
            "unsupported",
            "seed",
            "synthetic",
            &path,
            PluginConfig {
                api_version,
                ..Default::default()
            },
        );
        assert!(unsupported
            .err()
            .unwrap()
            .to_string()
            .contains("unsupported Lua plugin api_version"));
    }

    let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
    let url = format!("http://{}", listener.local_addr().unwrap());
    let server = std::thread::spawn(move || {
        for _ in 0..2 {
            let (mut socket, _) = listener.accept().unwrap();
            socket
                .set_read_timeout(Some(std::time::Duration::from_secs(3)))
                .unwrap();
            let mut request = Vec::new();
            let mut chunk = [0; 1024];
            while !request.windows(4).any(|part| part == b"\r\n\r\n") {
                let size = socket.read(&mut chunk).unwrap();
                assert_ne!(size, 0);
                request.extend_from_slice(&chunk[..size]);
                assert!(request.len() < 64 * 1024);
            }
            let body = br#"{"ok":true}"#;
            let head = format!("HTTP/1.1 200 OK\r\nContent-Length: {}\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n", body.len());
            socket.write_all(head.as_bytes()).unwrap();
            socket.write_all(body).unwrap();
        }
    });
    let plain_path = script(
        &directory,
        &format!(
            r#"
        assert(qcm.installation==nil)
        local response=qcm.get_http_client():get({url:?}):send()
        assert(response:json().ok and response:status()==200)
        response:close()
    "#
        ),
    );
    let plain = LuaProvider::with_config(
        None,
        "plain",
        "seed",
        "synthetic",
        &plain_path,
        PluginConfig::default(),
    )
    .unwrap();
    plain.check(&call_context(&db)).await.unwrap();
    let v2_path = script(
        &directory,
        &format!(
            r#"
        local original=qcm.get_http_client():get({url:?}):response_limit(1024):send()
        local http=qcm.get_http_client()
        for _,name in ipairs {{'cookie','set_cookie','clear_cookies'}} do
            local ok,value=pcall(function() return http[name] end)
            assert(not ok or value==nil)
        end
        assert(qcm.url.decode_path==nil)
        assert(qcm.url.percent_decode('%00%FF+%252F')=='\0'..string.char(255)..'+%2F')
        assert(not pcall(qcm.url.percent_decode,'%2'))
        local parts,bytes=original:read_body {{max_bytes=512}}
        assert(type(parts)=='userdata' and type(bytes)=='string')
        assert(original:body_state()=='consumed' and original:status()==200)
        original:close()
        local response=parts:with_body(bytes)
        assert(type(response)=='userdata' and response~=original)
        assert(not pcall(function() return parts:with_body(bytes) end))
        assert(not pcall(function() return original:json() end))
        assert(response:json().ok and response:status()==200)
        response:close()
    "#
        ),
    );
    let v2 = LuaProvider::with_config(
        None,
        "v2",
        "seed",
        "synthetic",
        &v2_path,
        PluginConfig::default(),
    )
    .unwrap();
    let result = v2.check(&call_context(&db)).await;
    server.join().unwrap();
    result.unwrap();
    assert!(LuaProvider::with_context(
        None,
        "test",
        "seed",
        "synthetic",
        &path,
        PluginConfig::default(),
        installation(&db, "other")
    )
    .is_err());
}

#[test]
fn manifest_requires_current_api_and_rejects_old_fields() {
    let directory = Directory::new();
    let path = directory.0.join("plugin.json");
    let mut value = serde_json::json!({
        "name": "TEST", "script_path": "provider.lua", "svg_path": "icon.svg",
        "auth_types": [], "api_version": API_VERSION,
    });
    std::fs::write(&path, serde_json::to_vec(&value).unwrap()).unwrap();
    assert_eq!(
        PluginManifest::read(&path).unwrap().config().api_version,
        API_VERSION
    );
    for version in [
        serde_json::json!(0),
        serde_json::json!(API_VERSION + 1),
        serde_json::json!("1"),
        serde_json::Value::Null,
    ] {
        value["api_version"] = version;
        std::fs::write(&path, serde_json::to_vec(&value).unwrap()).unwrap();
        assert!(PluginManifest::read(&path).is_err());
    }
    value.as_object_mut().unwrap().remove("api_version");
    std::fs::write(&path, serde_json::to_vec(&value).unwrap()).unwrap();
    assert!(PluginManifest::read(&path)
        .err()
        .unwrap()
        .to_string()
        .contains("api_version"));
    value["api_version"] = API_VERSION.into();
    value["requires"] = serde_json::json!(["http.response.v2", "url.v1"]);
    std::fs::write(&path, serde_json::to_vec(&value).unwrap()).unwrap();
    assert!(PluginManifest::read(&path)
        .err()
        .unwrap()
        .to_string()
        .contains("unknown field `requires`"));
}
