use qcm_core::model::*;
use sea_orm_migration::prelude::*;


#[derive(DeriveMigrationName)]
pub struct Migration;

#[async_trait::async_trait]
impl MigrationTrait for Migration {
    async fn up(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        manager
            .create_table(
                Table::create()
                    .table(provider::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(provider::Column::ProviderId)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(ColumnDef::new(provider::Column::Name).string().not_null())
                    .col(ColumnDef::new(provider::Column::Type).string().not_null())
                    .col(
                        ColumnDef::new(provider::Column::BaseUrl)
                            .string()
                            .not_null(),
                    )
                    .col(ColumnDef::new(provider::Column::AuthMethod).json())
                    .col(ColumnDef::new(provider::Column::Cookie).string().not_null())
                    .col(ColumnDef::new(provider::Column::Custom).string().not_null())
                    .col(
                        ColumnDef::new(provider::Column::EditTime)
                            .timestamp()
                            .not_null()
                            .default(Expr::current_timestamp()),
                    )
                    .to_owned(),
            )
            .await?;
        Ok(())
    }

    async fn down(&self, _manager: &SchemaManager) -> Result<(), DbErr> {
        Ok(())
    }
}
