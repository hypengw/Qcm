use qcm_core::model::*;
use sea_orm_migration::prelude::*;
use sea_query;

use crate::{unique_index, unique_index_name};

#[derive(DeriveMigrationName)]
pub struct Migration;

#[async_trait::async_trait]
impl MigrationTrait for Migration {
    async fn up(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        manager
            .create_table(
                sea_query::Table::create()
                    .table(cache::Entity)
                    .col(
                        ColumnDef::new(cache::Column::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(ColumnDef::new(cache::Column::Key).string().not_null())
                    .col(
                        ColumnDef::new(cache::Column::CacheType)
                            .integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(cache::Column::ContentType)
                            .string()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(cache::Column::ContentLength)
                            .big_unsigned()
                            .not_null(),
                    )
                    .col(ColumnDef::new(cache::Column::Blob).binary())
                    .col(
                        ColumnDef::new(cache::Column::Timestamp)
                            .timestamp()
                            .not_null()
                            .default(Expr::current_timestamp()),
                    )
                    .col(
                        ColumnDef::new(cache::Column::LastUse)
                            .timestamp()
                            .not_null()
                            .default(Expr::current_timestamp()),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_index(unique_index!(cache::Entity, cache::Column::Key))
            .await?;

        Ok(())
    }

    async fn down(&self, _manager: &SchemaManager) -> Result<(), DbErr> {
        Ok(())
    }
}
