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
                Table::create()
                    .table(library::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(library::Column::LibraryId)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(ColumnDef::new(library::Column::Name).string().not_null())
                    .col(
                        ColumnDef::new(library::Column::ProviderId)
                            .big_integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(library::Column::NativeId)
                            .string()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(library::Column::EditTime)
                            .timestamp()
                            .not_null()
                            .default(Expr::current_timestamp()),
                    )
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_library_provider")
                            .from(library::Entity, library::Column::ProviderId)
                            .to(provider::Entity, provider::Column::ProviderId)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;
        manager
            .create_index(unique_index!(
                library::Entity,
                library::Column::ProviderId,
                library::Column::NativeId
            ))
            .await?;
        Ok(())
    }

    async fn down(&self, _manager: &SchemaManager) -> Result<(), DbErr> {
        Ok(())
    }
}
