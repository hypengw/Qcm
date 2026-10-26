use super::*;
use qcm_plugin_lua::provider::LuaProvider;
use sea_orm::{ConnectionTrait, Database, Schema};
use std::sync::Arc;

fn provider(id: i64, callback: &str) -> (Arc<LuaProvider>, std::path::PathBuf) {
    let path = std::env::temp_dir().join(format!("qcm-share-api-{}.lua", uuid::Uuid::new_v4()));
    std::fs::write(
        &path,
        format!(
            r#"
local p = {{}}
function p.load() end
function p.save() return '' end
function p.check() error('share must not authenticate') end
function p.login() error('share must not authenticate') end
function p.sync() error('share must not sync') end
function p.sync_item() error('share must not sync') end
function p.favorite() end
function p.image() end
function p.audio() error('share must not prepare audio') end
function p.subtitle() end
{callback}
return p
"#
        ),
    )
    .unwrap();
    (
        Arc::new(LuaProvider::new(Some(id), "share", "test", "share", &path).unwrap()),
        path,
    )
}
async fn request(ctx: &Context, id: i64, kind: ItemType) -> Result<Option<String>, ProcessError> {
    link(
        ctx,
        &msg::GetShareLinkReq {
            id,
            item_type: kind as i32,
        },
    )
    .await
    .map(|rsp| rsp.url)
}

#[tokio::test]
async fn share_links_resolve_typed_items_and_remote_mix_ids() {
    const PROVIDER: i64 = 819205;
    let db = Database::connect("sqlite::memory:").await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=OFF")
        .await
        .unwrap();
    let schema = Schema::new(db.get_database_backend());
    for table in [
        schema.create_table_from_entity(m::item::Entity),
        schema.create_table_from_entity(m::mix::Entity),
    ] {
        db.execute(db.get_database_backend().build(&table))
            .await
            .unwrap();
    }
    db.execute_unprepared(&format!(
        "INSERT INTO item(id,native_id,provider_id,type,create_at,update_at,last_sync_at) VALUES
         (11,'9007199254740993',{PROVIDER},101,0,0,0),(12,'21',{PROVIDER},51,0,0,0),
         (13,'22',{PROVIDER},53,0,0,0),(91,'wrong-namespace',{PROVIDER},101,0,0,0),
         (501,'23',{PROVIDER},54,0,0,0),(502,'recommend/daily',{PROVIDER},54,0,0,0);
         INSERT INTO mix(id,name,description,track_count,mix_type,remote_id) VALUES
         (91,'Remote','',0,2,501),(92,'Local','',0,0,NULL),(93,'Daily','',0,2,502);"
    ))
    .await
    .unwrap();
    let (events, _) = tokio::sync::mpsc::channel(1);
    let ctx = Context {
        db: db.clone(),
        cache_db: db,
        ev_sender: events,
        sync_coordinator: Default::default(),
    };
    let (plugin, path) = provider(
        PROVIDER,
        r#"
function p.share_url(id, kind)
    if id == 'recommend/daily' then return nil end
    local paths = {[101]='song',[51]='album',[53]='artist',[54]='playlist'}
    return 'https://music.163.com/' .. assert(paths[kind]) .. '?id=' .. id
end
"#,
    );
    global::add_provider(plugin);
    let _cleanup = scopeguard::guard((PROVIDER, path), |(id, path)| {
        global::remove_provider(id);
        let _ = std::fs::remove_file(path);
    });
    for (id, kind, suffix) in [
        (11, ItemType::Song, "song?id=9007199254740993"),
        (12, ItemType::Album, "album?id=21"),
        (13, ItemType::Artist, "artist?id=22"),
        (13, ItemType::AlbumArtist, "artist?id=22"),
        (91, ItemType::Mix, "playlist?id=23"),
    ] {
        assert_eq!(
            request(&ctx, id, kind).await.unwrap(),
            Some(format!("https://music.163.com/{suffix}"))
        );
    }
    assert!(request(&ctx, 92, ItemType::Mix).await.unwrap().is_none());
    assert!(request(&ctx, 93, ItemType::Mix).await.unwrap().is_none());
    assert!(request(&ctx, 11, ItemType::Radio).await.unwrap().is_none());
    assert!(matches!(
        request(&ctx, 11, ItemType::Artist).await,
        Err(ProcessError::WrongId(_))
    ));
    assert!(request(&ctx, 0, ItemType::Song).await.is_err());
    assert!(request(&ctx, 999, ItemType::Song).await.is_err());
    for callback in [
        "function p.share_url() return 'javascript:alert(1)' end",
        "function p.share_url() return 'file:///tmp/private' end",
        "function p.share_url() return 'https://user:secret@example.invalid/song' end",
    ] {
        let (invalid, path) = provider(PROVIDER, callback);
        global::add_provider(invalid);
        assert!(request(&ctx, 11, ItemType::Song).await.is_err());
        std::fs::remove_file(path).unwrap();
    }
    let (unsupported, path) = provider(PROVIDER, "");
    global::add_provider(unsupported);
    assert!(request(&ctx, 11, ItemType::Song).await.unwrap().is_none());
    std::fs::remove_file(path).unwrap();
}
