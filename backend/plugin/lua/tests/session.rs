use qcm_core::{
    db::sync_run::SyncCoordinator,
    provider::{Context, Provider, ProviderSession},
};
use qcm_plugin_lua::{
    plugin::PluginConfig,
    provider::LuaProvider,
};
use sea_orm::*;
use std::sync::Arc;

async fn database() -> DatabaseConnection {
    let db = Database::connect("sqlite::memory:").await.unwrap();
    db.execute_unprepared("CREATE TABLE provider(provider_id INTEGER PRIMARY KEY, custom TEXT NOT NULL, cookie TEXT NOT NULL, session_key TEXT NOT NULL)").await.unwrap();
    db
}
fn provider(script: &str) -> (LuaProvider, std::path::PathBuf) {
    let path = std::env::temp_dir().join(format!("qcm-session-{}.lua", uuid::Uuid::new_v4()));
    std::fs::write(
        &path,
        format!(
            r#"
local value='initial'
local p={{}}
function p.save() return value end
function p.load(s) value=s end
function p.resource_scope() return 'account-scope' end
function p.login() return {{type='Ok'}} end
function p.sync() end
function p.sync_item() end
function p.favorite() return true end
function p.image() end
function p.audio() end
function p.subtitle() end
function p.check() {script} end
return p
"#
        ),
    )
    .unwrap();
    let config = PluginConfig::default();
    let p = LuaProvider::with_config(Some(1), "test", "test-device", "test", &path, config)
        .unwrap();
    (p, path)
}
fn context(db: DatabaseConnection) -> Context {
    let (ev_sender, _) = tokio::sync::mpsc::channel(1);
    Context {
        db: db.clone(),
        cache_db: db,
        ev_sender,
        sync_coordinator: Arc::new(SyncCoordinator::default()),
    }
}
#[tokio::test]
async fn session_save_stages_then_persists_and_rejects_replacement() {
    let db = database().await;
    let (p,path)=provider("value='latest'; local result=qcm.inner:save_session(); assert(result=='staged' or result=='saved')");
    let ctx = context(db.clone());
    p.check(&ctx).await.unwrap();
    p.persist_session().await.unwrap();
    assert_eq!(p.save(), "latest");
    let key = p.session_key().unwrap();
    db.execute(Statement::from_sql_and_values(
        DbBackend::Sqlite,
        "INSERT INTO provider VALUES(1,?,?,?)",
        ["initial".into(), "[]".into(), key.clone().into()],
    ))
    .await
    .unwrap();
    p.bind_session(db.clone(), key);
    p.persist_session().await.unwrap();
    let row = db
        .query_one(Statement::from_string(
            DbBackend::Sqlite,
            "SELECT custom,cookie FROM provider",
        ))
        .await
        .unwrap()
        .unwrap();
    assert_eq!(row.try_get::<String>("", "custom").unwrap(), "latest");
    assert!(!row.try_get::<String>("", "cookie").unwrap().is_empty());
    assert_eq!(
        p.resource_scope().unwrap().as_deref(),
        Some("account-scope")
    );
    db.execute_unprepared("UPDATE provider SET custom='replacement', session_key='replacement'")
        .await
        .unwrap();
    assert!(p.persist_session().await.is_err());
    assert!(p.check(&ctx).await.is_err());
    let row = db
        .query_one(Statement::from_string(
            DbBackend::Sqlite,
            "SELECT custom FROM provider",
        ))
        .await
        .unwrap()
        .unwrap();
    assert_eq!(row.try_get::<String>("", "custom").unwrap(), "replacement");
    db.execute_unprepared("DELETE FROM provider").await.unwrap();
    assert!(p.check(&ctx).await.is_err());
    p.invalidate();
    assert!(p.check(&ctx).await.is_err());
    std::fs::remove_file(path).unwrap();
}
#[tokio::test]
async fn session_cookie_only_update_survives_reconstruction() {
    let db = database().await;
    let (p, path) = provider("qcm.inner:save_session()");
    let key = p.session_key().unwrap();
    db.execute(Statement::from_sql_and_values(
        DbBackend::Sqlite,
        "INSERT INTO provider VALUES(1,?,?,?)",
        ["initial".into(), "[]".into(), key.clone().into()],
    ))
    .await
    .unwrap();
    p.bind_session(db.clone(), key);
    p.load_cookie(r#"[{"raw_cookie":"login_token=synthetic; Path=/","path":["/",true],"domain":{"Suffix":"example.invalid"},"expires":{"AtUtc":"2099-01-01T00:00:00Z"}}]"#);
    p.check(&context(db.clone())).await.unwrap();
    let row = db
        .query_one(Statement::from_string(
            DbBackend::Sqlite,
            "SELECT cookie,custom FROM provider",
        ))
        .await
        .unwrap()
        .unwrap();
    let (restored, other) = provider("assert(value=='initial')");
    restored.load_cookie(&row.try_get::<String>("", "cookie").unwrap());
    restored.load(&row.try_get::<String>("", "custom").unwrap());
    assert_eq!(restored.save_cookie(), p.save_cookie());
    assert_eq!(
        restored.resource_scope().unwrap(),
        p.resource_scope().unwrap()
    );
    for p in [path, other] {
        std::fs::remove_file(p).unwrap();
    }
}
#[tokio::test]
async fn session_storage_failure_is_observable() {
    let db = database().await;
    let (p, path) = provider("qcm.inner:save_session()");
    p.bind_session(db.clone(), p.session_key().unwrap());
    db.execute_unprepared("DROP TABLE provider").await.unwrap();
    assert!(p.persist_session().await.is_err());
    assert!(p.check(&context(db)).await.is_err());
    std::fs::remove_file(path).unwrap();
}
#[tokio::test]
async fn session_cookie_rotation_isolates_late_responses() {
    use tokio::io::{AsyncReadExt, AsyncWriteExt};
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    let base = format!("http://{}", listener.local_addr().unwrap());
    let server = tokio::spawn(async move {
        let (mut socket, _) = listener.accept().await.unwrap();
        let mut data = [0; 2048];
        socket.read(&mut data).await.unwrap();
        tokio::time::sleep(std::time::Duration::from_millis(60)).await;
        socket.write_all(b"HTTP/1.1 200 OK\r\nSet-Cookie: session=old; Path=/\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}").await.unwrap();
    });
    let (p, path) = provider(&format!(
        r#"
local http=qcm.get_http_client()
local b=http:new_batch{{results='response',limit=1}}
b:add('old',http:get('{base}'):build());b:seal()
qcm.time.sleep(15)
qcm.inner:begin_session()
b:wait_one();b:close()
"#
    ));
    p.check(&context(database().await)).await.unwrap();
    server.await.unwrap();
    assert!(!p.save_cookie().contains("session=old"));
    std::fs::remove_file(path).unwrap();
}
#[tokio::test]
async fn shared_session_task_and_cancellation_release_waiters() {
    let (p, path) = provider(
        r#"
local result=qcm.inner:session_task('test',function() qcm.time.sleep(50);return 'done' end)
assert(result=='done')
"#,
    );
    let ctx = context(database().await);
    let (a, b) = tokio::join!(p.check(&ctx), p.check(&ctx));
    a.unwrap();
    b.unwrap();
    // Dropping the owner must release the other coroutine instead of leaving a busy flag.
    let owner = p.check(&ctx);
    let waiter = p.check(&ctx);
    tokio::pin!(owner);
    tokio::pin!(waiter);
    tokio::select! { _=&mut owner => panic!("completed too soon"), _=tokio::time::sleep(std::time::Duration::from_millis(5))=>{} }
    p.invalidate();
    assert!(
        tokio::time::timeout(std::time::Duration::from_secs(1), waiter)
            .await
            .unwrap()
            .is_err()
    );
    std::fs::remove_file(path).unwrap();
}

#[tokio::test]
async fn session_concurrent_snapshots_do_not_roll_back() {
    let db = database().await;
    let (p, path) = provider("value=tostring((tonumber(value) or 0)+1); qcm.inner:save_session()");
    let key = p.session_key().unwrap();
    db.execute(Statement::from_sql_and_values(
        DbBackend::Sqlite,
        "INSERT INTO provider VALUES(1,?,?,?)",
        ["initial".into(), "[]".into(), key.clone().into()],
    ))
    .await
    .unwrap();
    p.bind_session(db.clone(), key);
    let ctx = context(db.clone());
    let (a, b, c) = tokio::join!(p.check(&ctx), p.check(&ctx), p.check(&ctx));
    a.unwrap();
    b.unwrap();
    c.unwrap();
    let row = db
        .query_one(Statement::from_string(
            DbBackend::Sqlite,
            "SELECT custom FROM provider",
        ))
        .await
        .unwrap()
        .unwrap();
    assert_eq!(row.try_get::<String>("", "custom").unwrap(), "3");
    std::fs::remove_file(path).unwrap();
}

#[tokio::test]
async fn authentication_challenge_keeps_structured_details() {
    use qcm_core::error::ProviderError;
    let db = database().await;
    let (p,path)=provider("qcm.error{code='not_auth',message='verification needed',jump_url='https://example.invalid/verify'}");
    let error = p.check(&context(db)).await.unwrap_err();
    let mut cause = &error;
    while let ProviderError::WithContext { err, .. } = cause {
        cause = err;
    }
    assert!(
        matches!(cause,ProviderError::AuthRequired{message,jump_url} if message=="verification needed" && jump_url.as_deref()==Some("https://example.invalid/verify"))
    );
    std::fs::remove_file(path).unwrap();
}
