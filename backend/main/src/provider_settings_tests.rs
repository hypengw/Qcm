use super::*;
use qcm_core::{
    db::provider_settings as store,
    provider::{Context, ProviderCommon},
};
use qcm_plugin_lua::{plugin::Capabilities, provider::LuaProvider};
use sea_orm::{ConnectionTrait, Database, DatabaseConnection, EntityTrait, Schema};

fn provider() -> Arc<dyn Provider> {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../plugin/lua/jellyfin");
    let caps: Capabilities =
        serde_json::from_slice(&std::fs::read(root.join("plugin.json")).unwrap()).unwrap();
    Arc::new(
        LuaProvider::with_capabilities(
            None,
            "Original",
            "settings-test",
            "jellyfin",
            &root.join("src/provider.lua"),
            caps,
        )
        .unwrap(),
    )
}

async fn database() -> DatabaseConnection {
    let mut options = sea_orm::ConnectOptions::new("sqlite::memory:");
    options.max_connections(1);
    let db = Database::connect(options).await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=ON")
        .await
        .unwrap();
    let schema = Schema::new(db.get_database_backend());
    db.execute(
        db.get_database_backend()
            .build(&schema.create_table_from_entity(qcm_core::model::provider::Entity)),
    )
    .await
    .unwrap();
    migration::MigrationTrait::up(
        &migration::m20261019_000001_provider_settings::Migration,
        &migration::SchemaManager::new(&db),
    )
    .await
    .unwrap();
    db
}

#[tokio::test]
async fn provider_settings_commit_restore_reject_and_replace_authentication() {
    let db = database().await;
    let original = provider();
    let id = crate::db::add_provider(&db, original.clone())
        .await
        .unwrap();
    global::add_provider(original.clone());
    let initial = read(id).unwrap();
    assert_eq!(initial.revision, 0);
    assert_eq!(
        serde_json::from_str::<serde_json::Value>(&initial.values_json).unwrap()["max_bitrate"],
        320000
    );
    let cleanup = scopeguard::guard((), |_| global::remove_provider(id));
    let directory = std::env::temp_dir().join(format!("qcm-settings-{}", uuid::Uuid::new_v4()));
    let cache = crate::resource_cache::ResourceCache::open(directory.clone())
        .await
        .unwrap();
    let (events, _) = tokio::sync::mpsc::channel(1);
    let context = Arc::new(Context {
        db: db.clone(),
        cache_db: db.clone(),
        ev_sender: events,
        sync_coordinator: Default::default(),
    });
    let (backend_ev, _) = tokio::sync::mpsc::channel(1);
    let (oper, _) = crate::task::TaskManager::new();
    let service = Arc::new(ServiceContext {
        provider_context: context,
        backend_ev,
        oper,
        mix_refresh: Default::default(),
        playback: Default::default(),
        radio: Default::default(),
        resource_cache: cache.clone(),
    });
    let mut req = msg::UpdateProviderReq {
        provider_id: id,
        name: "Edited".into(),
        settings_json: r#"{"playback_mode":"original","max_bitrate":192000}"#.into(),
        ..Default::default()
    };
    let epoch = original.lifecycle_epoch();
    assert_eq!(
        update(&service, &req).await.unwrap().code,
        msg::model::AuthResult::Ok as i32
    );
    assert!(Arc::ptr_eq(&global::provider(id).unwrap(), &original));
    assert_eq!(original.lifecycle_epoch(), epoch);
    assert_eq!(original.settings().revision, 1);
    assert_eq!(original.settings().values["max_bitrate"], 192000);
    assert_eq!(original.name(), "Edited");
    let direct = original
        .open_playback(qcm_core::provider::PlaybackRequest {
            item_id: "unused".into(),
            max_bitrate: None,
        })
        .await
        .unwrap();
    assert_eq!(direct.kind, qcm_core::provider::PlaybackKind::ByteResource);

    req.name = "Must not publish".into();
    assert!(update(&service, &req).await.is_err());
    assert_eq!(original.name(), "Edited");
    assert_eq!(store::load(&db, id).await.unwrap().revision, 1);
    req.settings_revision = 1;
    req.settings_json = r#"{"max_bitrate":123}"#.into();
    assert!(update(&service, &req)
        .await
        .unwrap()
        .field_errors
        .contains_key("max_bitrate"));
    assert_eq!(original.name(), "Edited");

    db.execute_unprepared("CREATE TRIGGER reject_settings_name BEFORE UPDATE OF name ON provider WHEN NEW.name='Rejected' BEGIN SELECT RAISE(ABORT,'test write failure'); END").await.unwrap();
    req.name = "Rejected".into();
    req.settings_json = r#"{"playback_mode":"hls","max_bitrate":320000}"#.into();
    assert!(update(&service, &req).await.is_err());
    assert_eq!(original.name(), "Edited");
    assert_eq!(original.settings().revision, 1);
    assert_eq!(
        store::load(&db, id).await.unwrap().values,
        original.settings().values
    );
    db.execute_unprepared("DROP TRIGGER reject_settings_name")
        .await
        .unwrap();

    let restored = provider();
    restored.set_id(Some(id));
    store::restore(&db, restored.as_ref()).await.unwrap();
    assert_eq!(restored.settings().values, original.settings().values);
    let key = format!("settings-{}", uuid::Uuid::new_v4());
    global::set_tmp_provider(&key, restored.clone());
    req.tmp_provider = key.clone();
    req.name = "Reauthenticated".into();
    req.settings_json = serde_json::to_string(&original.settings().values).unwrap();
    assert_eq!(
        update(&service, &req).await.unwrap().code,
        msg::model::AuthResult::Ok as i32
    );
    assert!(Arc::ptr_eq(&global::provider(id).unwrap(), &restored));
    assert_eq!(restored.settings().revision, 2);
    assert_eq!(restored.settings().values, original.settings().values);
    global::remove_tmp_provider(&key);

    let before = restored.settings();
    let txn = db.begin().await.unwrap();
    store::save(&txn, id, before.revision, Default::default())
        .await
        .unwrap();
    txn.rollback().await.unwrap();
    assert_eq!(store::load(&db, id).await.unwrap().values, before.values);
    qcm_core::model::provider::Entity::delete_by_id(id)
        .exec(&db)
        .await
        .unwrap();
    assert_eq!(store::load(&db, id).await.unwrap().revision, 0);
    drop(cleanup);
    cache.shutdown().await.unwrap();
    let _ = std::fs::remove_dir_all(directory);
}
