use migration::sea_orm::{ConnectionTrait, Database, DbBackend, Statement};
use migration::{Migrator, MigratorTrait};
#[tokio::test]
async fn provider_session_migration_preserves_legacy_authentication() {
    let db = Database::connect("sqlite::memory:").await.unwrap();
    Migrator::up(&db, Some(1)).await.unwrap();
    db.execute_unprepared("INSERT INTO provider(name,type,base_url,cookie,custom) VALUES('test','netease','','synthetic-cookie','synthetic-state')").await.unwrap();
    migration::MigrationTrait::up(
        &migration::m20261017_000001_provider_session::Migration,
        &migration::SchemaManager::new(&db),
    )
    .await
    .unwrap();
    let row = db
        .query_one(Statement::from_string(
            DbBackend::Sqlite,
            "SELECT session_key,cookie,custom FROM provider",
        ))
        .await
        .unwrap()
        .unwrap();
    assert_eq!(row.try_get::<String>("", "session_key").unwrap(), "");
    assert_eq!(
        row.try_get::<String>("", "cookie").unwrap(),
        "synthetic-cookie"
    );
    assert_eq!(
        row.try_get::<String>("", "custom").unwrap(),
        "synthetic-state"
    );
}
