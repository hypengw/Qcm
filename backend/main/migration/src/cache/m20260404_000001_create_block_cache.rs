use qcm_core::model::*;
use sea_orm_migration::prelude::*;
use sea_query;

use crate::{unique_index, unique_index_name};

#[derive(DeriveMigrationName)]
pub struct Migration;

#[async_trait::async_trait]
impl MigrationTrait for Migration {
    async fn up(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        // Create cache_source table
        manager
            .create_table(
                sea_query::Table::create()
                    .table(cache_source::Entity)
                    .col(
                        ColumnDef::new(cache_source::Column::Key)
                            .string()
                            .not_null()
                            .primary_key(),
                    )
                    .col(
                        ColumnDef::new(cache_source::Column::CacheType)
                            .integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(cache_source::Column::ContentType)
                            .string()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(cache_source::Column::ContentLength)
                            .big_integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(cache_source::Column::BlockCount)
                            .integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(cache_source::Column::Timestamp)
                            .timestamp()
                            .not_null()
                            .default(Expr::current_timestamp()),
                    )
                    .col(
                        ColumnDef::new(cache_source::Column::LastUse)
                            .timestamp()
                            .not_null()
                            .default(Expr::current_timestamp()),
                    )
                    .to_owned(),
            )
            .await?;

        // Create cache_block table
        manager
            .create_table(
                sea_query::Table::create()
                    .table(cache_block::Entity)
                    .col(
                        ColumnDef::new(cache_block::Column::Key)
                            .string()
                            .not_null()
                            .primary_key(),
                    )
                    .col(
                        ColumnDef::new(cache_block::Column::SourceKey)
                            .string()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(cache_block::Column::BlockIndex)
                            .integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(cache_block::Column::BlockSize)
                            .big_integer()
                            .not_null(),
                    )
                    .col(ColumnDef::new(cache_block::Column::Blob).binary())
                    .col(
                        ColumnDef::new(cache_block::Column::Timestamp)
                            .timestamp()
                            .not_null()
                            .default(Expr::current_timestamp()),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_index(
                unique_index!(
                    cache_block::Entity,
                    cache_block::Column::SourceKey,
                    cache_block::Column::BlockIndex
                ),
            )
            .await?;

        Ok(())
    }

    async fn down(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        manager
            .drop_table(sea_query::Table::drop().table(cache_block::Entity).to_owned())
            .await?;
        manager
            .drop_table(
                sea_query::Table::drop()
                    .table(cache_source::Entity)
                    .to_owned(),
            )
            .await?;
        Ok(())
    }
}
