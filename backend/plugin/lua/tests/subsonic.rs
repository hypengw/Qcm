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
    std::env::var_os("SUBSONIC_PLUGIN_ROOT")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("subsonic"))
}
fn provider() -> LuaProvider {
    let caps: Capabilities =
        serde_json::from_slice(&std::fs::read(root().join("plugin.json")).unwrap()).unwrap();
    LuaProvider::with_capabilities(
        Some(1),
        "Subsonic test",
        "qcm-sdk-migration-test",
        "subsonic",
        &root().join("src/provider.lua"),
        caps,
    )
    .unwrap()
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
    migration::MigrationTrait::up(
        &migration::m20261017_000001_provider_session::Migration,
        &migration::SchemaManager::new(&db),
    )
    .await
    .unwrap();
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
        name: Set("Subsonic test".into()),
        type_: Set("subsonic".into()),
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
async fn manifest_and_metadata_do_not_access_network() {
    let provider = provider();
    let ctx = context().await;
    assert_eq!(provider.save(), r#"{"version":1}"#);
    let cards = provider.for_you_items(&ctx).await.unwrap();
    assert_eq!(cards.len(), 1);
    let definition = provider.queue_definition(&ctx, "random").await.unwrap();
    assert!(definition.can_restart);
    assert!(definition.configuration.is_empty());
    assert!(provider.check(&ctx).await.is_err());
}

fn credentials() -> AuthInfo {
    AuthInfo {
        server_url: std::env::var("SUBSONIC_URL").unwrap(),
        method: Some(AuthMethod::Username {
            username: std::env::var("SUBSONIC_USER").unwrap(),
            pw: std::env::var("SUBSONIC_PASSWORD").unwrap(),
        }),
    }
}

#[derive(Default)]
struct ServerState {
    classic: bool,
    get_only: bool,
    missing_page: bool,
    duplicate_page: bool,
    duplicate_playlist: bool,
    failing_extensions: bool,
    plain_lyrics: bool,
    empty_lyrics: bool,
    empty_random: bool,
    favorites: std::collections::HashSet<String>,
    requests: Vec<(String, String, Vec<(String, String)>)>,
}
struct Server {
    base: String,
    state: Arc<std::sync::Mutex<ServerState>>,
    task: tokio::task::JoinHandle<()>,
}
impl Drop for Server {
    fn drop(&mut self) {
        self.task.abort();
    }
}
fn artist() -> serde_json::Value {
    serde_json::json!({"id":"artist/one", "name":"艺人", "albumCount":1})
}
fn album() -> serde_json::Value {
    serde_json::json!({"id":"album/one", "name":"专辑", "songCount":2, "duration":6,
        "artistId":"artist/one", "artist":"艺人", "coverArt":"cover independent",
        "created":"2026-10-18T00:00:00.123456789Z", "releaseDate":{"year":2020}})
}
fn songs() -> Vec<serde_json::Value> {
    ["song/a +?", "song/b"].into_iter().enumerate().map(|(index,id)| serde_json::json!({
        "id":id,"title":format!("歌曲 {index}"),"duration":3,"albumId":"album/one","album":"专辑",
        "artistId":"artist/one","artist":"艺人","coverArt":"cover independent",
        "artists":[{"id":"guest","name":"客串"}],"albumArtists":[{"id":"artist/one","name":"艺人"}],
        "track":index+1,"discNumber":1,"isDir":false,"isVideo":false,"type":"music",
    })).collect()
}
fn reply(
    state: &mut ServerState,
    method: &str,
    path: &str,
    params: Vec<(String, String)>,
) -> (u16, &'static str, Vec<u8>) {
    let business: Vec<_> = params
        .into_iter()
        .filter(|(name, _)| !["u", "t", "s", "v", "c", "f"].contains(&name.as_str()))
        .collect();
    let param = |key: &str| {
        business
            .iter()
            .find(|(name, _)| name == key)
            .map(|(_, v)| v.as_str())
    };
    let mut root = serde_json::json!({"status":"ok","version":"1.16.1","type":"test","openSubsonic":!state.classic});
    let mut code = 200;
    match path {
        "ping" => {}
        "getOpenSubsonicExtensions" => {
            if state.failing_extensions {
                code = 503;
            }
            root["openSubsonicExtensions"] = serde_json::json!([
                {"name":"songLyrics","versions":[1]},
                {"name":if state.get_only {"unused"} else {"formPost"},"versions":[1]}
            ]);
        }
        "getScanStatus" => {
            code = 404;
        }
        "getMusicFolders" => {
            root["musicFolders"] = serde_json::json!({"musicFolder":[{"id":1,"name":"音乐库"}]})
        }
        "getArtists" => {
            root["artists"] = serde_json::json!({"index":[{"name":"A","artist":[artist()]}]})
        }
        "getAlbumList2" => {
            root["albumList2"] = serde_json::json!({"album":if param("offset").unwrap_or("0")=="0" {vec![album()]} else {vec![]}})
        }
        "search3" => {
            let group = ["artist", "album", "song"]
                .into_iter()
                .find(|g| param(&format!("{g}Count")).unwrap_or("0") != "0")
                .unwrap();
            let offset = param(&format!("{group}Offset"))
                .unwrap_or("0")
                .parse::<usize>()
                .unwrap();
            let rows = match group {
                "artist" => vec![artist()],
                "album" => vec![album()],
                _ => songs(),
            };
            let page: Vec<_> = rows
                .into_iter()
                .skip(if state.duplicate_page { 0 } else { offset })
                .take(1)
                .collect();
            if !(state.missing_page && group == "song") {
                root["searchResult3"] = serde_json::json!({group:page});
            }
        }
        "getAlbum" => {
            root["album"] = album();
            root["album"]["song"] = serde_json::json!(songs());
        }
        "getArtist" => {
            root["artist"] = artist();
            root["artist"]["album"] = serde_json::json!([album()]);
        }
        "getSong" => {
            let mut song = songs()
                .into_iter()
                .find(|s| s["id"].as_str() == param("id"))
                .unwrap();
            if state.favorites.contains(param("id").unwrap()) {
                song["starred"] = serde_json::json!("2026-10-18T00:00:00Z");
            }
            root["song"] = song;
        }
        "getStarred2" => {
            let starred: Vec<_> = songs()
                .into_iter()
                .filter(|s| state.favorites.contains(s["id"].as_str().unwrap()))
                .collect();
            root["starred2"] = serde_json::json!({"song":starred});
        }
        "star" | "unstar" => {
            let id = param("id")
                .or_else(|| param("albumId"))
                .or_else(|| param("artistId"))
                .unwrap();
            if path == "star" {
                state.favorites.insert(id.into());
            } else {
                state.favorites.remove(id);
            }
        }
        "getPlaylists" => {
            root["playlists"] = serde_json::json!({"playlist":[{"id":"playlist","name":"歌单","songCount":2,"duration":6,"created":"2026-10-18T00:00:00Z","changed":"2026-10-18T00:00:00Z"}]})
        }
        "getPlaylist" => {
            let mut rows = songs();
            if state.duplicate_playlist {
                rows[1] = rows[0].clone();
            }
            root["playlist"] = serde_json::json!({"id":"playlist","name":"歌单","songCount":2,"duration":6,"created":"2026-10-18T00:00:00Z","changed":"2026-10-18T00:00:00Z","entry":rows});
        }
        "getRandomSongs" => {
            root["randomSongs"] = if state.empty_random {
                serde_json::json!({})
            } else {
                serde_json::json!({"song":songs()})
            }
        }
        "getLyricsBySongId" => {
            root["lyricsList"] = serde_json::json!({"structuredLyrics":[
                {"lang":"zh","synced":!state.plain_lyrics,"offset":100,"line":[{"start":500,"value":"第一行"},{"start":1000,"value":"第二行"}]}
            ]})
        }
        "getLyrics" => {
            root["lyrics"] =
                serde_json::json!({"value":if state.empty_lyrics {""} else {"纯文本歌词"}})
        }
        "stream" | "getCoverArt" => {
            if param("id") == Some("missing-json") {
                root = serde_json::json!({"status":"failed","version":"1.16.1","error":{"code":70,"message":"Missing media"}});
            } else if param("id") == Some("missing-xml") {
                state.requests.push((method.into(), path.into(), business));
                return (
                    200,
                    "text/xml",
                    b"<subsonic-response status=\"failed\"/>".to_vec(),
                );
            } else {
                state.requests.push((method.into(), path.into(), business));
                return (
                    200,
                    if path == "stream" {
                        "audio/mpeg"
                    } else {
                        "image/jpeg"
                    },
                    vec![1, 2, 3, 4],
                );
            }
        }
        _ => {
            code = 404;
        }
    }
    state.requests.push((method.into(), path.into(), business));
    (
        code,
        "application/json",
        serde_json::to_vec(&serde_json::json!({"subsonic-response":root})).unwrap(),
    )
}
impl Server {
    async fn new() -> Self {
        use tokio::io::{AsyncReadExt, AsyncWriteExt};
        let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
        let base = format!("http://{}/subpath", listener.local_addr().unwrap());
        let state = Arc::new(std::sync::Mutex::new(ServerState::default()));
        let shared = state.clone();
        let task = tokio::spawn(async move {
            loop {
                let (mut socket, _) = listener.accept().await.unwrap();
                let mut data = Vec::new();
                let mut chunk = [0; 4096];
                let (header, body_start) = loop {
                    let n = socket.read(&mut chunk).await.unwrap();
                    if n == 0 {
                        break (String::new(), 0);
                    }
                    data.extend_from_slice(&chunk[..n]);
                    if let Some(end) = data.windows(4).position(|s| s == b"\r\n\r\n") {
                        break (String::from_utf8(data[..end].to_vec()).unwrap(), end + 4);
                    }
                };
                if header.is_empty() {
                    continue;
                }
                let length = header
                    .lines()
                    .find_map(|l| {
                        l.to_lowercase()
                            .strip_prefix("content-length:")
                            .map(|v| v.trim().parse::<usize>().unwrap())
                    })
                    .unwrap_or(0);
                while data.len() < body_start + length {
                    let n = socket.read(&mut chunk).await.unwrap();
                    assert!(n > 0);
                    data.extend_from_slice(&chunk[..n]);
                }
                let mut words = header.lines().next().unwrap().split_whitespace();
                let method = words.next().unwrap();
                let target = words.next().unwrap();
                let url = reqwest::Url::parse(&format!("http://localhost{target}")).unwrap();
                assert!(url.path().starts_with("/subpath/rest/"));
                let path = url.path().rsplit('/').next().unwrap();
                let params = if method == "POST" {
                    assert!(header
                        .to_lowercase()
                        .contains("application/x-www-form-urlencoded"));
                    reqwest::Url::parse(&format!(
                        "http://localhost/?{}",
                        String::from_utf8_lossy(&data[body_start..body_start + length])
                    ))
                    .unwrap()
                } else {
                    url.clone()
                };
                let params: Vec<_> = params
                    .query_pairs()
                    .map(|(k, v)| (k.into_owned(), v.into_owned()))
                    .collect();
                for name in ["u", "t", "s", "v", "c", "f"] {
                    assert_eq!(params.iter().filter(|(k, _)| k == name).count(), 1);
                }
                let (status, mime, body) = reply(&mut shared.lock().unwrap(), method, path, params);
                let head=format!("HTTP/1.1 {status} Response\r\nContent-Type: {mime}\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",body.len());
                socket.write_all(head.as_bytes()).await.unwrap();
                socket.write_all(&body).await.unwrap();
            }
        });
        Self { base, state, task }
    }
    async fn login(&self, ctx: &Context) -> LuaProvider {
        let p = provider();
        let info = AuthInfo {
            server_url: self.base.clone(),
            method: Some(AuthMethod::Username {
                username: "mock user".into(),
                pw: "not persisted".into(),
            }),
        };
        assert!(matches!(p.auth(ctx, &info).await.unwrap(), AuthResult::Ok));
        p
    }
}

#[tokio::test]
async fn short_pages_restore_playlists_and_failed_sync_preserve_data() {
    use qcm_core::{
        model::type_enum::ItemType,
        provider::{QueueNextRequest, QueueOpenRequest},
    };
    let server = Server::new().await;
    let ctx = context().await;
    let p = server.login(&ctx).await;
    p.sync(&ctx).await.unwrap();
    assert_eq!(m::song::Entity::find().count(&ctx.db).await.unwrap(), 2);
    assert_eq!(m::artist::Entity::find().count(&ctx.db).await.unwrap(), 2);
    assert_eq!(
        m::album::Entity::find()
            .one(&ctx.db)
            .await
            .unwrap()
            .unwrap()
            .duration,
        6000
    );
    for song in m::song::Entity::find().all(&ctx.db).await.unwrap() {
        assert_eq!(song.duration, 3000);
    }
    let playlist = m::item::Entity::find()
        .filter(m::item::Column::Type.eq(ItemType::Mix))
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap();
    p.sync_item(&ctx, playlist.clone()).await.unwrap();
    let mix = m::mix::Entity::find()
        .filter(m::mix::Column::RemoteId.eq(playlist.id))
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap();
    let count = m::rel_mix_song::Entity::find()
        .filter(m::rel_mix_song::Column::MixId.eq(mix.id))
        .count(&ctx.db)
        .await
        .unwrap();
    assert_eq!(count, 2);
    server.state.lock().unwrap().duplicate_playlist = true;
    assert!(p.sync_item(&ctx, playlist).await.is_err());
    assert_eq!(
        m::rel_mix_song::Entity::find()
            .filter(m::rel_mix_song::Column::MixId.eq(mix.id))
            .count(&ctx.db)
            .await
            .unwrap(),
        count
    );
    for duplicate in [false, true] {
        {
            let mut state = server.state.lock().unwrap();
            state.missing_page = !duplicate;
            state.duplicate_page = duplicate;
        }
        assert!(p.sync(&ctx).await.is_err());
        assert_eq!(m::song::Entity::find().count(&ctx.db).await.unwrap(), 2);
    }
    {
        let mut state = server.state.lock().unwrap();
        state.missing_page = false;
        state.duplicate_page = false;
    }
    p.sync(&ctx).await.unwrap();
    let current = songs()
        .iter()
        .map(|s| s["id"].as_str().unwrap().to_string())
        .collect();
    let open = p
        .open_queue(&ctx, "random", QueueOpenRequest::default())
        .await
        .unwrap();
    let next = p
        .next_queue(
            &ctx,
            "random",
            QueueNextRequest {
                state: open.state,
                current_native_ids: current,
            },
        )
        .await
        .unwrap();
    assert!(next.exhausted && next.songs.is_empty());
    let calls = server
        .state
        .lock()
        .unwrap()
        .requests
        .iter()
        .filter(|(_, p, _)| p == "getRandomSongs")
        .count();
    assert_eq!(calls, 4);
    server.state.lock().unwrap().empty_random = true;
    assert!(
        p.open_queue(&ctx, "random", QueueOpenRequest::default())
            .await
            .unwrap()
            .exhausted
    );
    assert_eq!(
        p.subtitle("song/a +?").await.unwrap().items[0].start,
        Some(400)
    );
    server.state.lock().unwrap().plain_lyrics = true;
    assert_eq!(p.subtitle("song/a +?").await.unwrap().items[0].start, None);
    for (kind, id, field) in [
        (ItemType::Song, "song/a +?", "id"),
        (ItemType::Album, "album/one", "albumId"),
        (ItemType::Artist, "artist/one", "artistId"),
    ] {
        p.favorite(&ctx, id, kind, true).await.unwrap();
        assert!(server
            .state
            .lock()
            .unwrap()
            .requests
            .last()
            .unwrap()
            .2
            .contains(&(field.into(), id.into())));
        p.favorite(&ctx, id, kind, false).await.unwrap();
    }
}

#[tokio::test]
async fn classic_get_and_extension_failures_are_distinct() {
    let server = Server::new().await;
    let ctx = context().await;
    server.state.lock().unwrap().classic = true;
    let p = server.login(&ctx).await;
    p.sync(&ctx).await.unwrap();
    assert_eq!(m::song::Entity::find().count(&ctx.db).await.unwrap(), 2);
    assert!(server
        .state
        .lock()
        .unwrap()
        .requests
        .iter()
        .all(|(method, _, _)| method == "GET"));
    assert_eq!(
        p.subtitle("song/a +?").await.unwrap().items[0]
            .text
            .as_deref(),
        Some("纯文本歌词")
    );
    server.state.lock().unwrap().empty_lyrics = true;
    assert!(p.subtitle("song/a +?").await.unwrap().items.is_empty());
    {
        let mut state = server.state.lock().unwrap();
        state.classic = false;
        state.failing_extensions = true;
    }
    let p = provider();
    let info = AuthInfo {
        server_url: server.base.clone(),
        method: Some(AuthMethod::Username {
            username: "mock".into(),
            pw: "not persisted".into(),
        }),
    };
    assert!(p.auth(&ctx, &info).await.is_err());
}

#[tokio::test]
async fn restored_capability_discovery_is_shared() {
    let server = Server::new().await;
    let ctx = context().await;
    let original = server.login(&ctx).await;
    let restored = provider();
    restored.load_auth_info(&server.base, original.auth_method());
    restored.load(&original.save());
    server.state.lock().unwrap().requests.clear();
    let (one, two) = tokio::join!(restored.check(&ctx), restored.check(&ctx));
    one.unwrap();
    two.unwrap();
    let state = server.state.lock().unwrap();
    assert_eq!(
        state
            .requests
            .iter()
            .filter(|(_, path, _)| path == "ping")
            .count(),
        1
    );
    assert_eq!(
        state
            .requests
            .iter()
            .filter(|(_, path, _)| path == "getOpenSubsonicExtensions")
            .count(),
        1
    );
}

#[tokio::test]
async fn prepared_validation_rejects_json_xml_and_account_changes() {
    use qcm_core::{model::type_enum::ImageType, resource::Resource};
    let server = Server::new().await;
    let ctx = context().await;
    let p = server.login(&ctx).await;
    for id in ["missing-json", "missing-xml"] {
        for resource in [
            Resource::Audio { item_id: id.into() },
            Resource::Image {
                item_id: "song/a +?".into(),
                image_id: Some(id.into()),
                kind: ImageType::Primary,
            },
        ] {
            let prepared = p.prepare_resource(resource).await.unwrap();
            assert!(prepared.fetch(Default::default()).await.is_err());
        }
    }
    let prepared = p
        .prepare_resource(Resource::Audio {
            item_id: "song/a +?".into(),
        })
        .await
        .unwrap();
    let second = p
        .prepare_resource(Resource::Audio {
            item_id: "song/a +?".into(),
        })
        .await
        .unwrap();
    assert_ne!(prepared.url(), second.url());
    assert_eq!(
        prepared.cache_url(prepared.url()),
        second.cache_url(second.url())
    );
    assert_eq!(
        prepared
            .fetch(Default::default())
            .await
            .unwrap()
            .bytes()
            .await
            .unwrap()
            .as_ref(),
        [1, 2, 3, 4]
    );
    p.invalidate();
    assert!(prepared.fetch(Default::default()).await.is_err());
}

#[tokio::test]
#[ignore = "requires the authorized Subsonic test server and credentials in environment"]
async fn live_sync_restore_media_and_radio() {
    use qcm_core::{
        model::type_enum::{ImageType, ItemType},
        provider::{QueueNextRequest, QueueOpenRequest},
        resource::Resource,
    };
    let ctx = context().await;
    let p = provider();
    let info = credentials();
    assert!(matches!(p.auth(&ctx, &info).await.unwrap(), AuthResult::Ok));
    for _ in 0..2 {
        let restored = provider();
        restored.load_auth_info(&info.server_url, p.auth_method());
        restored.load(&p.save());
        restored.check(&ctx).await.unwrap();
        assert_eq!(restored.save(), r#"{"version":1}"#);
    }
    p.sync(&ctx).await.unwrap();
    let first = m::item::Entity::find()
        .order_by_asc(m::item::Column::Id)
        .all(&ctx.db)
        .await
        .unwrap();
    p.sync(&ctx).await.unwrap();
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
        "Subsonic live sync: artists={}, albums={}, songs={}, radios={}",
        m::artist::Entity::find().count(&ctx.db).await.unwrap(),
        m::album::Entity::find().count(&ctx.db).await.unwrap(),
        m::song::Entity::find().count(&ctx.db).await.unwrap(),
        first.iter().filter(|i| i.r#type == ItemType::Radio).count()
    );
    let song = first.iter().find(|i| i.r#type == ItemType::Song).unwrap();
    let mut headers = reqwest::header::HeaderMap::new();
    headers.insert("range", "bytes=0-511".parse().unwrap());
    let audio = p
        .prepare_resource(Resource::Audio {
            item_id: song.native_id.clone(),
        })
        .await
        .unwrap();
    let mut response = audio.fetch(headers).await.unwrap();
    assert_eq!(response.status(), 206);
    assert!(response.headers().contains_key("content-range"));
    assert!(!response.chunk().await.unwrap().unwrap().is_empty());
    let image = m::image::Entity::find()
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap();
    let image_item = first.iter().find(|i| i.id == image.item_id).unwrap();
    let prepared = p
        .prepare_resource(Resource::Image {
            item_id: image_item.native_id.clone(),
            image_id: image.native_id,
            kind: ImageType::Primary,
        })
        .await
        .unwrap();
    let mut response = prepared.fetch(Default::default()).await.unwrap();
    assert!(response.headers()["content-type"]
        .to_str()
        .unwrap()
        .starts_with("image/"));
    assert!(!response.chunk().await.unwrap().unwrap().is_empty());
    for resource in [
        Resource::Audio {
            item_id: "__qcm_missing__".into(),
        },
        Resource::Image {
            item_id: song.native_id.clone(),
            image_id: Some("__qcm_missing__".into()),
            kind: ImageType::Primary,
        },
    ] {
        let prepared = p.prepare_resource(resource).await.unwrap();
        assert!(prepared.fetch(Default::default()).await.is_err());
    }
    let batch = p
        .open_queue(&ctx, "random", QueueOpenRequest::default())
        .await
        .unwrap();
    assert!(!batch.songs.is_empty());
    let mut current = Vec::new();
    for song in &batch.songs {
        current.push(
            m::item::Entity::find_by_id(song.id)
                .one(&ctx.db)
                .await
                .unwrap()
                .unwrap()
                .native_id,
        );
    }
    let next = p
        .next_queue(
            &ctx,
            "random",
            QueueNextRequest {
                state: batch.state,
                current_native_ids: current.clone(),
            },
        )
        .await
        .unwrap();
    assert!(!next.songs.is_empty());
    for song in next.songs {
        let item = m::item::Entity::find_by_id(song.id)
            .one(&ctx.db)
            .await
            .unwrap()
            .unwrap();
        assert!(!current.contains(&item.native_id));
    }
    p.sync(&ctx).await.unwrap();
    assert_eq!(provider().for_you_items(&ctx).await.unwrap().len(), 1);
    let was_favorite = m::dynamic::Entity::find_by_id(song.id)
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap()
        .favorite_at
        .is_some();
    let changed = async {
        p.favorite(&ctx, &song.native_id, ItemType::Song, !was_favorite)
            .await?;
        p.sync_item(&ctx, song.clone()).await?;
        Ok::<_, qcm_core::error::ProviderError>(
            m::dynamic::Entity::find_by_id(song.id)
                .one(&ctx.db)
                .await?
                .unwrap()
                .favorite_at
                .is_some(),
        )
    }
    .await;
    let restore = p
        .favorite(&ctx, &song.native_id, ItemType::Song, was_favorite)
        .await;
    restore.unwrap();
    assert_eq!(changed.unwrap(), !was_favorite);
    p.sync_item(&ctx, song.clone()).await.unwrap();
    let lyrics = p.subtitle(&song.native_id).await.unwrap();
    println!("Subsonic restore, prepared media, error isolation, radio and restored favorite passed; lyric lines={}", lyrics.items.len());
    p.invalidate();
    assert!(!audio.is_valid());
}
