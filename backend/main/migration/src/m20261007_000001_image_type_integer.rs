use sea_orm_migration::prelude::*;
use sea_orm_migration::sea_orm::{DatabaseBackend, Statement, TransactionTrait};

#[derive(DeriveMigrationName)]
pub struct Migration;
#[async_trait::async_trait]
impl MigrationTrait for Migration {
    async fn up(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        let transaction = manager.get_connection().begin().await?;
        let db = &transaction;
        let invalid=db.query_one(Statement::from_string(DatabaseBackend::Sqlite,
            "SELECT COUNT(*) AS count FROM image WHERE image_type NOT IN ('0','1','2','3','4','Primary','Backdrop','Banner','Thumb','Logo')".to_owned())).await?.unwrap();
        if invalid.try_get::<i64>("", "count")? != 0 {
            return Err(DbErr::Custom(
                "image contains unknown image_type values".into(),
            ));
        }
        db.execute_unprepared("CREATE TABLE image_integer (
            id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
            item_id BIGINT NOT NULL REFERENCES item(id) ON DELETE CASCADE ON UPDATE CASCADE,
            image_type INTEGER NOT NULL,
            native_id TEXT, db TEXT, fresh TEXT NOT NULL, timestamp TIMESTAMP NOT NULL,
            UNIQUE(item_id,image_type)
        );
        INSERT INTO image_integer SELECT id,item_id,
            CASE image_type WHEN 'Primary' THEN 0 WHEN 'Backdrop' THEN 1 WHEN 'Banner' THEN 2 WHEN 'Thumb' THEN 3 WHEN 'Logo' THEN 4 ELSE CAST(image_type AS INTEGER) END,
            native_id,db,fresh,timestamp FROM image;
        DROP TABLE image;
        ALTER TABLE image_integer RENAME TO image;").await?;
        transaction.commit().await?;
        Ok(())
    }
    async fn down(&self, _manager: &SchemaManager) -> Result<(), DbErr> {
        Ok(())
    }
}
