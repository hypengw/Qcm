use migration::{Migrator, MigratorTrait};
use qcm_core::{
    db::{fts, sync_run::SyncCoordinator},
    model as m,
    provider::{AuthInfo, AuthMethod, AuthResult, Context, Provider, ProviderCommon},
};
use qcm_plugin_lua::{plugin::Capabilities, provider::LuaProvider};
use sea_orm::*;
use std::{path::PathBuf, sync::Arc};

fn root() -> PathBuf {
    std::env::var_os("JELLYFIN_PLUGIN_ROOT")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("jellyfin"))
}
fn provider() -> LuaProvider {
    let caps: Capabilities =
        serde_json::from_slice(&std::fs::read(root().join("plugin.json")).unwrap()).unwrap();
    LuaProvider::with_capabilities(
        Some(1),
        "Jellyfin test",
        "qcm-sdk-migration-test",
        "jellyfin",
        &root().join("src/provider.lua"),
        caps,
    )
    .unwrap()
}

#[tokio::test]
#[ignore = "requires the authorized Jellyfin test server and credentials in environment"]
async fn live_hls_negotiation_and_scoped_cleanup() {
    use qcm_core::provider::{PlaybackKind,PlaybackRequest};
    let p=Arc::new(provider());let ctx=context().await;
    let info=AuthInfo {server_url:std::env::var("JELLYFIN_URL").unwrap(),method:Some(AuthMethod::Username {username:std::env::var("JELLYFIN_USER").unwrap(),pw:std::env::var("JELLYFIN_PASSWORD").unwrap()})};
    assert!(matches!(p.auth(&ctx,&info).await.unwrap(),AuthResult::Ok));
    let item_id=std::env::var("JELLYFIN_ITEM").unwrap();
    let source=p.open_playback(PlaybackRequest {item_id,max_bitrate:Some(128000)}).await.unwrap();
    let active=p.clone();let playback=source.clone();
    let result=tokio::spawn(async move {
        let p=active;let source=playback;
        assert_eq!(source.kind,PlaybackKind::HlsVod);
        let prepared=p.prepare_playback_resource(&source.state,&source.uri).await.unwrap();
        assert_eq!(prepared.cache_mode(),qcm_core::resource::CacheMode::Document);
        let response=prepared.fetch(Default::default()).await.unwrap();assert_eq!(response.status(),200);
        let text=response.text().await.unwrap();assert!(text.starts_with("#EXTM3U"));
        assert!(p.prepare_playback_resource(&source.state,"http://example.test/foreign.ts").await.is_err());
        p.keep_playback(&source.state).await.unwrap();
    }).await;
    p.close_playback(&source.state).await.unwrap();
    let saved:serde_json::Value=serde_json::from_str(&p.save()).unwrap();
    let logout=reqwest::Client::new().post(format!("{}/Sessions/Logout",std::env::var("JELLYFIN_URL").unwrap())).header("x-emby-token",saved["token"].as_str().unwrap()).send().await.unwrap();
    assert_eq!(logout.status(),204);
    result.unwrap();
}
async fn context() -> Context {
    let mut options = ConnectOptions::new("sqlite::memory:");
    options.max_connections(1);
    let db = Database::connect(options).await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=ON")
        .await
        .unwrap();
    Migrator::up(&db, Some(4)).await.unwrap();
    migration::MigrationTrait::up(
        &migration::m20261007_000001_image_type_integer::Migration,
        &migration::SchemaManager::new(&db),
    )
    .await
    .unwrap();
    migration::MigrationTrait::up(&migration::m20261017_000001_provider_session::Migration, &migration::SchemaManager::new(&db)).await.unwrap();
    for (table, columns) in [
        ("album", vec!["name", "description"]),
        ("artist", vec!["name", "description"]),
        ("song", vec!["name"]),
        ("mix", vec!["name", "description"]),
    ] {
        db.execute_unprepared(
            &fts::create_fts_table_sql(table, &columns)
                .replace("tokenize='qcm'", "tokenize='unicode61'"),
        )
        .await
        .unwrap();
        fts::create_fts_triggers(&db, table, &columns)
            .await
            .unwrap();
    }
    m::provider::ActiveModel {
        provider_id: Set(1),
        name: Set("Jellyfin test".into()),
        type_: Set("jellyfin".into()),
        base_url: Set(String::new()),
        auth_method: Set(None),
        cookie: Set(String::new()),
        session_key: Set(String::new()),
        custom: Set(String::new()),
        edit_time: Set(chrono::Utc::now()),
    }
    .insert(&db)
    .await
    .unwrap();
    let (sender, mut receiver) = tokio::sync::mpsc::channel(128);
    tokio::spawn(async move { while receiver.recv().await.is_some() {} });
    Context {
        cache_db: db.clone(),
        db,
        ev_sender: sender,
        sync_coordinator: Arc::new(SyncCoordinator::default()),
    }
}
#[tokio::test]
async fn capabilities_and_optional_queue() {
    let mut caps = Capabilities::default();
    caps.requires = vec!["unknown.v9".into()];
    assert!(caps.validate().is_err());
    let p = provider();
    let ctx = context().await;
    assert!(matches!(
        p.get_queue_next(&ctx, "", &[]).await,
        Err(qcm_core::error::ProviderError::NotImplemented)
    ));
    p.load("{invalid");
    assert_eq!(p.save(), "{invalid");
}

#[tokio::test]
async fn prepared_resources_accept_304_and_reject_invalidated_accounts() {
    use qcm_core::{resource::Resource,model::type_enum::ImageType};
    use tokio::io::{AsyncReadExt,AsyncWriteExt};
    let listener=tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    let base=format!("http://{}",listener.local_addr().unwrap());
    let p=provider(); p.load_auth_info(&base,None);
    p.load(r#"{"token":"test-token","user_id":"00000000000000000000000000000001"}"#);
    let request=p.prepare_resource(Resource::Image {item_id:"00000000000000000000000000000002".into(),image_id:None,kind:ImageType::Primary}).await.unwrap();
    assert!(request.url().path().ends_with("/Images/Primary"));
    assert!(request.headers().contains_key("authorization"));
    let server=tokio::spawn(async move {
        let (mut socket,_)=listener.accept().await.unwrap();
        let mut data=Vec::new(); let mut chunk=[0;1024];
        while !data.ends_with(b"\r\n\r\n") {let n=socket.read(&mut chunk).await.unwrap(); assert!(n>0); data.extend_from_slice(&chunk[..n]);}
        assert!(String::from_utf8(data).unwrap().to_ascii_lowercase().contains("if-none-match: \"one\""));
        socket.write_all(b"HTTP/1.1 304 Not Modified\r\nETag: \"one\"\r\nConnection: close\r\n\r\n").await.unwrap();
    });
    let mut headers=reqwest::header::HeaderMap::new(); headers.insert("if-none-match","\"one\"".parse().unwrap());
    assert_eq!(request.fetch(headers).await.unwrap().status(),304);
    server.await.unwrap(); p.invalidate(); assert!(!request.is_valid());
    assert!(matches!(request.fetch(Default::default()).await,Err(qcm_core::error::ProviderError::NotAuth)));
}
#[tokio::test]
#[ignore = "requires the authorized Jellyfin test server and credentials in environment"]
async fn live_sync_restore_and_media() {
    let base = std::env::var("JELLYFIN_URL").unwrap();
    let ctx = context().await;
    let p = provider();
    let info = AuthInfo {
        server_url: base.clone(),
        method: Some(AuthMethod::Username {
            username: std::env::var("JELLYFIN_USER").unwrap(),
            pw: std::env::var("JELLYFIN_PASSWORD").unwrap(),
        }),
    };
    assert!(matches!(p.auth(&ctx, &info).await.unwrap(), AuthResult::Ok));
    p.check(&ctx).await.unwrap();
    let current: serde_json::Value = serde_json::from_str(&p.save()).unwrap();
    let saved = serde_json::json!({"token":current["token"]}).to_string();
    let restored = provider();
    restored.load_auth_info(&base, None);
    restored.load(&saved);
    restored.check(&ctx).await.unwrap();
    let again = provider();
    again.load_auth_info(&base, None);
    again.load(&restored.save());
    again.check(&ctx).await.unwrap();
    again.sync(&ctx).await.unwrap();
    let first = m::item::Entity::find()
        .order_by_asc(m::item::Column::Id)
        .all(&ctx.db)
        .await
        .unwrap();
    assert!(!first.is_empty());
    again.sync(&ctx).await.unwrap();
    let second = m::item::Entity::find()
        .order_by_asc(m::item::Column::Id)
        .all(&ctx.db)
        .await
        .unwrap();
    assert_eq!(
        first
            .iter()
            .map(|i| (i.id, &i.native_id, i.create_at))
            .collect::<Vec<_>>(),
        second
            .iter()
            .map(|i| (i.id, &i.native_id, i.create_at))
            .collect::<Vec<_>>()
    );
    println!(
        "live sync: libraries={}, artists={}, albums={}, songs={}, mixes={}",
        m::library::Entity::find().count(&ctx.db).await.unwrap(),
        m::artist::Entity::find().count(&ctx.db).await.unwrap(),
        m::album::Entity::find().count(&ctx.db).await.unwrap(),
        m::song::Entity::find().count(&ctx.db).await.unwrap(),
        m::remote_mix::Entity::find().count(&ctx.db).await.unwrap()
    );
    let song = first
        .iter()
        .find(|i| i.r#type == m::type_enum::ItemType::Song)
        .unwrap();
    let mut headers = reqwest::header::HeaderMap::new();
    headers.insert(
        "Range",
        reqwest::header::HeaderValue::from_static("bytes=0-511"),
    );
    let mut audio = again
        .audio(&ctx, &song.native_id, Some(headers))
        .await
        .unwrap();
    assert_eq!(audio.status(), 206);
    assert!(audio.headers().contains_key("content-range"));
    assert!(!audio.chunk().await.unwrap().unwrap().is_empty());
    let image = m::image::Entity::find()
        .filter(m::image::Column::ImageType.eq(m::type_enum::ImageType::Primary))
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap();
    let item = first.iter().find(|i| i.id == image.item_id).unwrap();
    let mut response = again
        .image(
            &ctx,
            &item.native_id,
            image.native_id.as_deref(),
            image.image_type,
        )
        .await
        .unwrap();
    assert!(response.status().is_success());
    assert!(!response.chunk().await.unwrap().unwrap().is_empty());
    let mixes: Vec<_> = first
        .iter()
        .filter(|i| i.r#type == m::type_enum::ItemType::Mix)
        .collect();
    for mix in mixes {
        again.sync_item(&ctx, mix.clone()).await.unwrap();
    }
    let favorite = m::dynamic::Entity::find_by_id(song.id)
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap()
        .favorite_at
        .is_some();
    again
        .favorite(&ctx, &song.native_id, song.r#type, !favorite)
        .await
        .unwrap();
    let changed = again.sync_item(&ctx, song.clone()).await;
    let observed = m::dynamic::Entity::find_by_id(song.id)
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap()
        .favorite_at
        .is_some();
    let restore = again
        .favorite(&ctx, &song.native_id, song.r#type, favorite)
        .await;
    restore.unwrap();
    changed.unwrap();
    assert_eq!(observed, !favorite);
    again.sync_item(&ctx, song.clone()).await.unwrap();
    println!("live restore twice, repeated IDs, audio Range, image, playlist refresh and restored favorite passed");
}

#[tokio::test]
#[ignore = "requires NCM_PLUGIN_ROOT pointing to the NCM checkout"]
async fn ncm_batch_and_legacy_context() {
    use std::io::{Read, Write};
    let root = PathBuf::from(std::env::var("NCM_PLUGIN_ROOT").unwrap());
    let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
    let base = format!("http://{}", listener.local_addr().unwrap());
    let server = std::thread::spawn(move || {
        for sequence in 0..27 {
            let (mut stream, _) = listener.accept().unwrap();
            let mut request = [0; 8192];
            let _ = stream.read(&mut request).unwrap();
            let body = format!("{{\"code\":200,\"sequence\":{sequence}}}");
            write!(stream,"HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{body}",body.len()).unwrap();
        }
    });
    let script = std::env::temp_dir().join(format!("qcm-ncm-test-{}.lua", std::process::id()));
    let prefix = format!(
        "package.path={}..';'..package.path\nlocal base={}\n",
        serde_json::to_string(&format!("{}/src/?.lua", root.display())).unwrap(),
        serde_json::to_string(&base).unwrap()
    );
    std::fs::write(&script,prefix+r#"
local provider=require('provider')
local Client=require('client')
local client=Client.new('test')
function provider.check()
    for _,count in ipairs{0,1,13,13} do
        local accepted,parsed=0,0
        local function next_api()
            if accepted==count then return nil end
            accepted=accepted+1
            return {
                base=function() return base end, operation=function() return 'GET' end,
                path=function() return '/test' end, crypto=function() return '' end,
                query=function() return {} end,
                parse_response=function(_,response)
                    parsed=parsed+1
                    assert(response:status()==200)
                    local body=response:json()
                    body.parsed=true
                    return body
                end,
            }
        end
        local results=client:perform_queue(next_api,5)
        assert(#results==count and parsed==count)
        for _,result in ipairs(results) do assert(result.parsed) end
    end
end
function provider.sync_item(ctx)
    local lib=ctx:sync_libraries{{library_id=-1,name='Legacy',provider_id=qcm.inner.id,native_id='legacy'}}[1]
    local ids=ctx:allocate_items{{id=-1,native_id='legacy-song',provider_id=qcm.inner.id,library_id=lib,type='Song'}}
    ctx:sync_songs{{id=ids[1],name='Legacy song',duration=42000,track_number=1,disc_number=1,can_play=true,popularity=0,tags={}}}
    ctx:sync_dynamics{{id=ids[1],is_external=true,favorite_at=123}}
end
return provider
"#).unwrap();
    let p = LuaProvider::new(Some(1), "NCM test", "test", "netease", &script).unwrap();
    let ctx = context().await;
    p.load(r#"{"user_id":123,"nickname":"test","avatar_url":""}"#);
    assert!(p.save().contains("123"));
    p.check(&ctx).await.unwrap();
    let dummy = m::item::Model {
        id: 0,
        native_id: String::new(),
        provider_id: 1,
        library_id: None,
        r#type: m::type_enum::ItemType::Song,
        create_at: Default::default(),
        update_at: Default::default(),
        last_sync_at: Default::default(),
    };
    p.sync_item(&ctx, dummy).await.unwrap();
    let song = m::song::Entity::find().one(&ctx.db).await.unwrap().unwrap();
    assert_eq!(song.duration, 42000);
    assert!(
        m::dynamic::Entity::find_by_id(song.id)
            .one(&ctx.db)
            .await
            .unwrap()
            .unwrap()
            .is_external
    );
    server.join().unwrap();
    std::fs::remove_file(script).unwrap();
}

#[tokio::test]
async fn failed_explicit_sync_rejects_retained_context_and_preserves_commits() {
    let path = std::env::temp_dir().join(format!("qcm-context-test-{}.lua", std::process::id()));
    std::fs::write(&path,r#"
local retained,mode
return {
 load=function(value) mode=value end,save=function() return '' end,
 check=function() end,login=function() return {type='Ok'} end,
 sync=function(ctx)
    retained=ctx
    local lib=ctx:sync_batch{libraries={{native_id='one',name='One'}}}.libraries[1].library_id
    local run=ctx:begin_sync{libraries={lib},scopes={{library_id=lib,item_type=qcm.enum.ItemType.Song},{item_type=qcm.enum.ItemType.Mix}}}
    run:sync_batch{songs={{item={library_id=lib,type='Song',native_id='committed'},name='Committed'}}}
    if mode=='caught' then
        assert(not pcall(run.sync_batch,run,{songs={{item=1,name='Bad',duration='invalid'}}}))
        run:complete('libraries')
        run:complete{library_id=lib,item_type=qcm.enum.ItemType.Song}
        run:complete{item_type=qcm.enum.ItemType.Mix}
        run:seal()
        return
    end
    error('interrupted after commit')
 end,
 sync_item=function(ctx) retained:sync_batch{libraries={{native_id='stale',name='Stale'}}} end,
 favorite=function() end,image=function() end,audio=function() end,
 subtitle=function()
    if mode=='lrc' then return '[00:01.50]line' end
    if mode=='bad' then return {items={{start=-1,text='bad'}}} end
    return {items={{start=1500,text='line'}}}
 end,
}
"#).unwrap();
    let caps = Capabilities {
        requires: vec!["db.explicit_sync.v1".into()],
        sync_mode: Some("explicit".into()),
        sync_types: vec![m::type_enum::ItemType::Song],
    };
    let p = LuaProvider::with_capabilities(Some(1), "test", "test", "test", &path, caps).unwrap();
    let ctx = context().await;
    assert!(p.sync(&ctx).await.is_err());
    let item = m::item::Entity::find().one(&ctx.db).await.unwrap().unwrap();
    assert_eq!(item.native_id, "committed");
    assert!(p.sync_item(&ctx, item).await.is_err());
    assert_eq!(m::library::Entity::find().count(&ctx.db).await.unwrap(), 1);
    p.load("caught");
    assert!(p.sync(&ctx).await.is_err());
    assert_eq!(m::song::Entity::find().count(&ctx.db).await.unwrap(), 1);
    let structured = p.subtitle("test").await.unwrap();
    assert_eq!(structured.items[0].start, Some(1500));
    p.load("lrc");
    let lrc = p.subtitle("test").await.unwrap();
    assert_eq!(lrc.items[0].start, Some(1500));
    p.load("bad");
    assert!(p.subtitle("test").await.is_err());
    std::fs::remove_file(path).unwrap();
}
