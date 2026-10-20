use super::*;
use qcm_core::{
    db::network as store,
    network::{ClientPolicy, NetworkSettings},
};
use sea_orm::{ConnectionTrait, Database, DatabaseConnection, EntityTrait, Schema};

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
        &migration::m20261019_000002_network_proxies::Migration,
        &migration::SchemaManager::new(&db),
    )
    .await
    .unwrap();
    for id in [9001, 9002] {
        use sea_orm::ActiveValue::Set;
        qcm_core::model::provider::Entity::insert(qcm_core::model::provider::ActiveModel {
            provider_id: Set(id),
            name: Set("Network".into()),
            type_: Set("test".into()),
            base_url: Set(String::new()),
            auth_method: Set(None),
            cookie: Set(String::new()),
            session_key: Set(String::new()),
            custom: Set(String::new()),
            edit_time: Set(chrono::Utc::now()),
        })
        .exec(&db)
        .await
        .unwrap();
    }
    db
}
fn request(revision: u64, name: &str, providers: &[i64]) -> msg::UpdateNetworkSettingsReq {
    msg::UpdateNetworkSettingsReq { revision, proxies_json: serde_json::json!([{
        "id":"3f6f7d6b-0b93-49c3-afcc-200000000001", "name":name, "url":"http://user:synthetic-secret@127.0.0.1:7890", "provider_ids": providers
    }]).to_string() }
}
#[tokio::test]
async fn network_settings_persist_conflict_rollback_restore_and_cascade() {
    let db = database().await;
    assert_eq!(read(&db).await.unwrap().revision, 0);
    update(&db, &request(0, "Shared", &[9001, 9002]))
        .await
        .unwrap();
    assert!(network::policy(Some(9001)) != ClientPolicy::default());
    let response = read(&db).await.unwrap();
    assert_eq!(response.revision, 1);
    let entries: Vec<ProxyEntry> = serde_json::from_str(&response.proxies_json).unwrap();
    assert_eq!(entries[0].provider_ids, vec![9001, 9002]);
    assert!(!entries[0].display_url.contains("synthetic-secret"));
    assert!(update(&db, &request(0, "Stale", &[9001])).await.is_err());
    assert_eq!(read(&db).await.unwrap().revision, 1);
    assert!(update(&db, &request(1, "Unknown", &[9999])).await.is_err());
    db.execute_unprepared("CREATE TRIGGER reject_proxy BEFORE INSERT ON network_proxy WHEN NEW.name='Rejected' BEGIN SELECT RAISE(ABORT,'synthetic failure'); END").await.unwrap();
    let error = update(&db, &request(1, "Rejected", &[9001]))
        .await
        .unwrap_err();
    assert!(!error.to_string().contains("synthetic-secret"));
    assert_eq!(store::load(&db).await.unwrap().revision, 1);
    assert_eq!(store::load(&db).await.unwrap().proxies[0].name, "Shared");
    assert!(network::policy(Some(9002)) != ClientPolicy::default());
    network::publish(network::compile(NetworkSettings::default()).unwrap());
    restore(&db).await.unwrap();
    assert!(network::policy(Some(9002)) != ClientPolicy::default());
    qcm_core::model::provider::Entity::delete_by_id(9001)
        .exec(&db)
        .await
        .unwrap();
    restore(&db).await.unwrap();
    assert!(network::policy(Some(9001)) == ClientPolicy::default());
    assert_eq!(
        store::load(&db).await.unwrap().proxies[0].provider_ids,
        vec![9002]
    );
    update(
        &db,
        &msg::UpdateNetworkSettingsReq {
            revision: 1,
            proxies_json: "[]".into(),
        },
    )
    .await
    .unwrap();
    assert!(network::policy(Some(9002)) == ClientPolicy::default());
    assert!(store::load(&db).await.unwrap().proxies.is_empty());
    assert_eq!(read(&db).await.unwrap().revision, 2);
}
