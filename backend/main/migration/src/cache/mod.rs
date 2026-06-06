pub use sea_orm_migration::{prelude::*, Migration, MigrationStatus};

mod m20220101_000001_create_table;
mod m20260404_000001_create_block_cache;
pub struct CacheDBMigrator;

const DROPED: &[&str] = &[];

#[async_trait::async_trait]
impl MigratorTrait for CacheDBMigrator {
    fn migrations() -> Vec<Box<dyn MigrationTrait>> {
        vec![
            Box::new(m20220101_000001_create_table::Migration),
            Box::new(m20260404_000001_create_block_cache::Migration),
        ]
    }

    async fn get_pending_migrations<C>(db: &C) -> Result<Vec<Migration>, DbErr>
    where
        C: ConnectionTrait,
    {
        Self::install(db).await?;
        Ok(Self::get_migration_with_status(db)
            .await?
            .into_iter()
            .filter(|file| {
                file.status() == MigrationStatus::Pending && !DROPED.contains(&file.name())
            })
            .collect())
    }

    async fn get_applied_migrations<C>(db: &C) -> Result<Vec<Migration>, DbErr>
    where
        C: ConnectionTrait,
    {
        Self::install(db).await?;
        Ok(Self::get_migration_with_status(db)
            .await?
            .into_iter()
            .filter(|file| {
                file.status() == MigrationStatus::Applied && DROPED.contains(&file.name())
            })
            .collect())
    }
}
