use sea_orm_migration::prelude::*;

use crate::{unique_index, unique_index_name};
use qcm_core::db::values::Timestamp;
use qcm_core::model::{item, mix, rel_mix_song, remote_mix, song};

#[derive(DeriveMigrationName)]
pub struct Migration;

impl Migration {}

fn timestamp_col<C>(c: C) -> ColumnDef
where
    C: IntoIden,
{
    ColumnDef::new(c)
        .big_integer()
        .default(Timestamp::now_expr())
        .not_null()
        .clone()
}

#[async_trait::async_trait]
impl MigrationTrait for Migration {
    async fn up(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        let db = manager.get_connection();

        {
            let stat = sea_orm::Statement::from_string(
                db.get_database_backend(),
                r#"
            CREATE UNIQUE INDEX item_native_type_provider_library_idx ON item (
                "native_id",
                "type",
                "provider_id",
                COALESCE("library_id", -1)
            );
            "#,
            );
            db.execute(stat).await?;
        }

        manager
            .create_table(
                Table::create()
                    .table(remote_mix::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(remote_mix::Column::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(ColumnDef::new(remote_mix::Column::Name).string().not_null())
                    .col(ColumnDef::new(remote_mix::Column::Description).string())
                    .col(ColumnDef::new(remote_mix::Column::MixType).string())
                    .col(ColumnDef::new(remote_mix::Column::TrackCount).integer())
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_remote_mix_item")
                            .from(remote_mix::Entity, remote_mix::Column::Id)
                            .to(item::Entity, item::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_table(
                Table::create()
                    .table(mix::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(mix::Column::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(ColumnDef::new(mix::Column::Name).string().not_null())
                    .col(ColumnDef::new(mix::Column::SortName).string())
                    .col(ColumnDef::new(mix::Column::MixType).integer().not_null())
                    .col(ColumnDef::new(mix::Column::RemoteId))
                    .col(ColumnDef::new(mix::Column::TrackCount).integer().not_null())
                    .col(ColumnDef::new(mix::Column::Description).string().not_null())
                    .col(
                        ColumnDef::new(mix::Column::ContentUpdateAt)
                            .big_integer()
                            .default(0)
                            .not_null(),
                    )
                    .col(timestamp_col(mix::Column::CreateAt))
                    .col(timestamp_col(mix::Column::UpdateAt))
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_mix-remote_mix")
                            .from(mix::Entity, mix::Column::RemoteId)
                            .to(remote_mix::Entity, remote_mix::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_index(
                Index::create()
                    .unique()
                    .name("idx_mix-remote_id")
                    .table(mix::Entity)
                    .col(mix::Column::RemoteId)
                    .to_owned(),
            )
            .await?;

        manager
            .create_table(
                Table::create()
                    .table(rel_mix_song::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(rel_mix_song::Column::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(
                        ColumnDef::new(rel_mix_song::Column::SongId)
                            .big_integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(rel_mix_song::Column::MixId)
                            .big_integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(rel_mix_song::Column::OrderIdx)
                            .big_integer()
                            .not_null(),
                    )
                    .col(timestamp_col(rel_mix_song::Column::UpdateAt))
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_rel_mix_song_song")
                            .from(rel_mix_song::Entity, rel_mix_song::Column::SongId)
                            .to(song::Entity, song::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_rel_mix_song_mix")
                            .from(rel_mix_song::Entity, rel_mix_song::Column::MixId)
                            .to(mix::Entity, mix::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_index(unique_index!(
                rel_mix_song::Entity,
                rel_mix_song::Column::MixId,
                rel_mix_song::Column::SongId
            ))
            .await?;

        Ok(())
    }

    async fn down(&self, _manager: &SchemaManager) -> Result<(), DbErr> {
        Ok(())
    }
}
