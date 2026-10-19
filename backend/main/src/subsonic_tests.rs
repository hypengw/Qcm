use crate::{
    event::ServiceContext,
    msg,
    radio::{RadioSessions, Target},
    resource_cache::{ResourceCache, ResourceRequest},
    task::TaskManager,
};
use http_body_util::BodyExt;
use migration::{Migrator, MigratorTrait};
use prost::Message;
use qcm_core::{
    db::{fts, sync_run::SyncCoordinator},
    global, model as m,
    model::type_enum::ImageType,
    provider::{AuthInfo, AuthMethod, Provider, ProviderCommon, QueueOpenRequest},
    resource::Resource,
};
use qcm_plugin_lua::{plugin::Capabilities, provider::LuaProvider};
use sea_orm::*;
use std::{path::PathBuf, sync::Arc};

#[tokio::test]
#[ignore = "requires the authorized Subsonic server and credentials in environment"]
async fn live_subsonic_home_radio_and_resource_cache() {
    const ID: i64 = 872149;
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../plugin/lua/subsonic");
    let caps: Capabilities =
        serde_json::from_slice(&std::fs::read(root.join("plugin.json")).unwrap()).unwrap();
    let p = Arc::new(
        LuaProvider::with_capabilities(
            Some(ID),
            "Subsonic",
            "subsonic-live",
            "subsonic",
            &root.join("src/provider.lua"),
            caps,
        )
        .unwrap(),
    );
    let mut options = ConnectOptions::new("sqlite::memory:");
    options.max_connections(1);
    let db = Database::connect(options).await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=ON")
        .await
        .unwrap();
    Migrator::up(&db, Some(4)).await.unwrap();
    for migration in [
        &migration::m20261007_000001_image_type_integer::Migration
            as &dyn migration::MigrationTrait,
        &migration::m20261017_000001_provider_session::Migration,
    ] {
        migration
            .up(&migration::SchemaManager::new(&db))
            .await
            .unwrap();
    }
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
        provider_id: Set(ID),
        name: Set("Subsonic".into()),
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
    let (ev_sender, mut receiver) = tokio::sync::mpsc::channel(128);
    let drain = tokio::spawn(async move { while receiver.recv().await.is_some() {} });
    let ctx = Arc::new(qcm_core::provider::Context {
        cache_db: db.clone(),
        db,
        ev_sender,
        sync_coordinator: Arc::new(SyncCoordinator::default()),
    });
    let info = AuthInfo {
        server_url: std::env::var("SUBSONIC_URL").unwrap(),
        method: Some(AuthMethod::Username {
            username: std::env::var("SUBSONIC_USER").unwrap(),
            pw: std::env::var("SUBSONIC_PASSWORD").unwrap(),
        }),
    };
    p.auth(&ctx, &info).await.unwrap();
    p.sync(&ctx).await.unwrap();
    global::add_provider(p.clone());
    let _registered = scopeguard::guard((), |_| global::remove_provider(ID));
    let directory = std::env::temp_dir().join(format!("qcm-subsonic-{}", uuid::Uuid::new_v4()));
    let _directory = scopeguard::guard(directory.clone(), |path| {
        let _ = std::fs::remove_dir_all(path);
    });
    let cache = ResourceCache::open(directory).await.unwrap();
    let radio = Arc::new(RadioSessions::default());
    let (oper, _) = TaskManager::new();
    let (backend_ev, _) = tokio::sync::mpsc::channel(1);
    let service = Arc::new(ServiceContext {
        provider_context: ctx.clone(),
        backend_ev,
        oper,
        radio: radio.clone(),
        resource_cache: cache.clone(),
    });
    let request = msg::QcmMessage {
        r#type: msg::MessageType::GetHomeRecommendationsReq as i32,
        payload: Some(msg::qcm_message::Payload::GetHomeRecommendationsReq(
            msg::GetHomeRecommendationsReq {
                provider_ids: vec![ID],
            },
        )),
        ..Default::default()
    };
    let snapshot = crate::api::process_qcm(&service, &request.encode_to_vec(), &mut None)
        .await
        .unwrap();
    let msg::qcm_message::Payload::GetHomeRecommendationsRsp(home) =
        snapshot.payload.as_ref().unwrap()
    else {
        panic!("wrong home response")
    };
    assert_eq!(home.providers.len(), 1);
    assert!(
        home.providers[0].error.is_empty(),
        "{}",
        home.providers[0].error
    );
    assert_eq!(home.providers[0].for_you.len(), 1);
    assert!(home.providers[0].radios.is_empty());
    let queues = crate::home::radio_queues(&ctx, &[ID]).await.unwrap();
    assert_eq!(queues.len(), 1);
    let target = Target {
        queue_id: queues[0].id,
        native_id: "random".into(),
        provider: p.clone(),
    };
    let page = radio
        .open(
            ctx.clone(),
            target,
            format!(
                "{}:{}",
                chrono::Utc::now().timestamp_millis(),
                uuid::Uuid::new_v4()
            ),
            String::new(),
            QueueOpenRequest::default(),
        )
        .await
        .unwrap();
    assert!(!page.batch.songs.is_empty());
    p.sync(&ctx).await.unwrap();
    let song = m::item::Entity::find_by_id(page.batch.songs[0].id)
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap();
    let scope = p.resource_scope().unwrap().unwrap();
    let fetch = |resource: Resource, kind| {
        let cache = cache.clone();
        let p = p.clone();
        let scope = scope.clone();
        async move {
            let prepared = p.prepare_resource(resource).await.unwrap();
            let mut headers = reqwest::header::HeaderMap::new();
            if kind == 1 {
                headers.insert("range", "bytes=0-511".parse().unwrap());
            }
            cache
                .response(ResourceRequest {
                    provider: ID,
                    kind,
                    scope,
                    prepared: Arc::new(prepared),
                    headers,
                })
                .await
        }
    };
    for _ in 0..2 {
        let response = fetch(
            Resource::Audio {
                item_id: song.native_id.clone(),
            },
            1,
        )
        .await
        .unwrap();
        assert_eq!(response.status(), 206);
        assert_eq!(
            response
                .into_body()
                .collect()
                .await
                .unwrap()
                .to_bytes()
                .len(),
            512
        );
    }
    let image = m::image::Entity::find()
        .filter(m::image::Column::ItemId.eq(song.id))
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap();
    let response = fetch(
        Resource::Image {
            item_id: song.native_id.clone(),
            image_id: image.native_id,
            kind: ImageType::Primary,
        },
        0,
    )
    .await
    .unwrap();
    assert!(response.headers()["content-type"]
        .to_str()
        .unwrap()
        .starts_with("image/"));
    assert!(!response
        .into_body()
        .collect()
        .await
        .unwrap()
        .to_bytes()
        .is_empty());
    let usage = cache.usage().await.unwrap();
    assert!(fetch(
        Resource::Audio {
            item_id: "__qcm_missing__".into()
        },
        1
    )
    .await
    .is_err());
    assert!(fetch(
        Resource::Image {
            item_id: song.native_id,
            image_id: Some("__qcm_missing__".into()),
            kind: ImageType::Primary
        },
        0
    )
    .await
    .is_err());
    assert_eq!(cache.usage().await.unwrap().audio, usage.audio);
    if let Some(path) = std::env::var_os("SUBSONIC_PROTOCOL_OUTPUT") {
        let path = PathBuf::from(path);
        std::fs::create_dir_all(&path).unwrap();
        std::fs::write(path.join("home.pb"), snapshot.encode_to_vec()).unwrap();
        let queues = msg::QcmMessage {
            r#type: msg::MessageType::GetRadioQueuesRsp as i32,
            payload: Some(msg::qcm_message::Payload::GetRadioQueuesRsp(
                msg::GetRadioQueuesRsp { queues },
            )),
            ..Default::default()
        };
        std::fs::write(path.join("entries.pb"), queues.encode_to_vec()).unwrap();
    }
    radio.close(&ctx, &page.session_id).await;
    cache.shutdown().await.unwrap();
    drain.abort();
    println!("Subsonic live home protocol, RadioSessions, post-sync Range, image cache and HTTP 200 errors passed");
}
