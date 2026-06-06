pub use sea_orm_migration::{prelude::*, Migration, MigrationStatus};

mod cache;
mod drop;
mod m20220101_000004_create_provider_table;
mod m20220101_000005_create_library_table;
mod m20250824_000001_create_table;
mod m20251129_145233_create_remote_mix;
mod m20251214_145233_create_fts_table;

pub struct Migrator;
pub use cache::CacheDBMigrator;

#[macro_export]
macro_rules! unique_index_name {
    ($entity:path, $($column:path),+) => {
        concat!(
            stringify!($entity), "_",
            $(stringify!($column), "_"),+
        ).trim_end_matches('_')
    };
}

#[macro_export]
macro_rules! unique_index {
    ($entity:path, $($column:path),+) => {
        sea_query::Index::create()
            .name(unique_index_name!($entity, $($column),+))
            .table($entity)
            $(.col($column))+
            .unique()
            .to_owned()
    };
}

const DROPED: &[&str] = &[];

#[async_trait::async_trait]
impl MigratorTrait for Migrator {
    fn migrations() -> Vec<Box<dyn MigrationTrait>> {
        vec![
            Box::new(m20220101_000004_create_provider_table::Migration),
            Box::new(m20220101_000005_create_library_table::Migration),
            Box::new(m20250824_000001_create_table::Migration),
            Box::new(m20251129_145233_create_remote_mix::Migration),
            Box::new(m20251214_145233_create_fts_table::Migration),
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
