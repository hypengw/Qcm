use sea_orm_migration::prelude::*;
#[derive(DeriveMigrationName)]
pub struct Migration;
#[async_trait::async_trait]
impl MigrationTrait for Migration {
    async fn up(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        let db = manager.get_connection();
        for sql in [
            "CREATE TABLE network_settings(id INTEGER PRIMARY KEY CHECK(id=1), revision BIGINT NOT NULL)",
            "INSERT INTO network_settings(id,revision) VALUES(1,0)",
            "CREATE TABLE network_proxy(proxy_id TEXT PRIMARY KEY NOT NULL,name TEXT NOT NULL,url TEXT NOT NULL,position INTEGER NOT NULL)",
            "CREATE TABLE provider_proxy(provider_id BIGINT PRIMARY KEY NOT NULL REFERENCES provider(provider_id) ON DELETE CASCADE,proxy_id TEXT NOT NULL REFERENCES network_proxy(proxy_id) ON DELETE CASCADE)",
        ] { db.execute_unprepared(sql).await?; }
        Ok(())
    }
    async fn down(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        for table in ["provider_proxy", "network_proxy", "network_settings"] {
            manager
                .get_connection()
                .execute_unprepared(&format!("DROP TABLE {table}"))
                .await?;
        }
        Ok(())
    }
}
