use qcm_core::{db::values::Timestamp, model::*};
use sea_orm_migration::prelude::*;
use sea_query;

use crate::{unique_index, unique_index_name};

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

#[derive(DeriveMigrationName)]
pub struct Migration;

#[async_trait::async_trait]
impl MigrationTrait for Migration {
    async fn up(&self, manager: &SchemaManager) -> Result<(), DbErr> {
        manager
            .create_table(
                Table::create()
                    .table(item::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(item::Column::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(ColumnDef::new(item::Column::NativeId).string().not_null())
                    .col(ColumnDef::new(item::Column::LibraryId).big_integer())
                    .col(
                        ColumnDef::new(item::Column::ProviderId)
                            .big_integer()
                            .not_null(),
                    )
                    .col(ColumnDef::new(item::Column::Type).integer().not_null())
                    .col(timestamp_col(item::Column::CreateAt))
                    .col(timestamp_col(item::Column::UpdateAt))
                    .col(timestamp_col(item::Column::LastSyncAt))
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_album_library")
                            .from(item::Entity, item::Column::LibraryId)
                            .to(library::Entity, library::Column::LibraryId)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_index(
                Index::create()
                    .name(unique_index_name!(item::Entity, item::Column::NativeId))
                    .table(item::Entity)
                    .col(item::Column::NativeId)
                    .to_owned(),
            )
            .await?;
        manager
            .create_index(
                Index::create()
                    .unique()
                    .name(unique_index_name!(
                        item::Entity,
                        item::Column::NativeId,
                        item::Column::Type,
                        item::Column::LibraryId
                    ))
                    .table(item::Entity)
                    .col(item::Column::NativeId)
                    .col(item::Column::Type)
                    .col(item::Column::LibraryId)
                    .and_where(Expr::col(item::Column::LibraryId).is_not_null())
                    .to_owned(),
            )
            .await?;
        manager
            .create_index(
                Index::create()
                    .unique()
                    .name(unique_index_name!(
                        item::Entity,
                        item::Column::NativeId,
                        item::Column::Type,
                        item::Column::ProviderId
                    ))
                    .table(item::Entity)
                    .col(item::Column::NativeId)
                    .col(item::Column::Type)
                    .col(item::Column::ProviderId)
                    .col(item::Column::LibraryId)
                    .to_owned(),
            )
            .await?;

        manager
            .create_table(
                Table::create()
                    .table(album::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(album::Column::Id)
                            .big_integer()
                            .not_null()
                            .primary_key(),
                    )
                    .col(ColumnDef::new(album::Column::Name).string().not_null())
                    .col(ColumnDef::new(album::Column::SortName).string())
                    .col(
                        ColumnDef::new(album::Column::TrackCount)
                            .integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(album::Column::DiscCount)
                            .integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(album::Column::Type)
                            .integer()
                            .default(type_enum::AlbumType::Album),
                    )
                    .col(ColumnDef::new(album::Column::Duration).integer().not_null())
                    .col(ColumnDef::new(album::Column::Description).string())
                    .col(ColumnDef::new(album::Column::Company).string())
                    .col(ColumnDef::new(album::Column::Language).string())
                    .col(ColumnDef::new(album::Column::PublishTime).big_integer())
                    .col(ColumnDef::new(album::Column::AddedAt).big_integer())
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_album_item_id")
                            .from(album::Entity, album::Column::Id)
                            .to(item::Entity, item::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_table(
                Table::create()
                    .table(artist::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(artist::Column::Id)
                            .big_integer()
                            .not_null()
                            .primary_key(),
                    )
                    .col(ColumnDef::new(artist::Column::Name).string().not_null())
                    .col(ColumnDef::new(artist::Column::SortName).string())
                    .col(
                        ColumnDef::new(artist::Column::Description)
                            .string()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(artist::Column::AlbumCount)
                            .integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(artist::Column::MusicCount)
                            .integer()
                            .not_null(),
                    )
                    .col(ColumnDef::new(artist::Column::AddedAt).big_integer())
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_artist_item_id")
                            .from(artist::Entity, artist::Column::Id)
                            .to(item::Entity, item::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;


        manager
            .create_table(
                Table::create()
                    .table(song::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(song::Column::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(ColumnDef::new(song::Column::Name).string().not_null())
                    .col(ColumnDef::new(song::Column::SortName).string())
                    .col(ColumnDef::new(song::Column::AlbumId).big_integer())
                    .col(
                        ColumnDef::new(song::Column::TrackNumber)
                            .integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(song::Column::DiscNumber)
                            .integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(song::Column::Duration)
                            .big_integer()
                            .not_null(),
                    )
                    .col(ColumnDef::new(song::Column::CanPlay).boolean().not_null())
                    .col(ColumnDef::new(song::Column::Tags).json().not_null())
                    .col(ColumnDef::new(song::Column::Popularity).double().not_null())
                    .col(ColumnDef::new(song::Column::PublishTime).big_integer())
                    .col(ColumnDef::new(album::Column::AddedAt).big_integer())
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_song_item")
                            .from(song::Entity, song::Column::Id)
                            .to(item::Entity, item::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_song_album")
                            .from(song::Entity, song::Column::AlbumId)
                            .to(album::Entity, album::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_table(
                Table::create()
                    .table(rel_album_artist::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(rel_album_artist::Column::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(
                        ColumnDef::new(rel_album_artist::Column::AlbumId)
                            .big_integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(rel_album_artist::Column::ArtistId)
                            .big_integer()
                            .not_null(),
                    )
                    .col(timestamp_col(rel_album_artist::Column::UpdateAt))
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_rel_album_artist_album")
                            .from(rel_album_artist::Entity, rel_album_artist::Column::AlbumId)
                            .to(album::Entity, album::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_rel_album_artist_artist")
                            .from(rel_album_artist::Entity, rel_album_artist::Column::ArtistId)
                            .to(artist::Entity, artist::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_index(unique_index!(
                rel_album_artist::Entity,
                rel_album_artist::Column::AlbumId,
                rel_album_artist::Column::ArtistId
            ))
            .await?;

        manager
            .create_table(
                Table::create()
                    .table(rel_song_artist::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(rel_song_artist::Column::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(
                        ColumnDef::new(rel_song_artist::Column::SongId)
                            .big_integer()
                            .not_null(),
                    )
                    .col(
                        ColumnDef::new(rel_song_artist::Column::ArtistId)
                            .big_integer()
                            .not_null(),
                    )
                    .col(timestamp_col(rel_song_artist::Column::UpdateAt))
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_rel_song_artist_song")
                            .from(rel_song_artist::Entity, rel_song_artist::Column::SongId)
                            .to(song::Entity, song::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk_rel_song_artist_artist")
                            .from(rel_song_artist::Entity, rel_song_artist::Column::ArtistId)
                            .to(artist::Entity, artist::Column::Id)
                            .on_delete(sea_query::ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_index(unique_index!(
                rel_song_artist::Entity,
                rel_song_artist::Column::SongId,
                rel_song_artist::Column::ArtistId
            ))
            .await?;


        manager
            .create_table(
                Table::create()
                    .table(dynamic::Entity)
                    .col(
                        ColumnDef::new(dynamic::Column::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(
                        ColumnDef::new(dynamic::Column::PlayCount)
                            .big_integer()
                            .not_null()
                            .default(0),
                    )
                    .col(
                        ColumnDef::new(dynamic::Column::RemotePlayCount)
                            .big_integer()
                            .not_null()
                            .default(0),
                    )
                    .col(
                        ColumnDef::new(dynamic::Column::IsExternal)
                            .boolean()
                            .not_null()
                            .default(false),
                    )
                    .col(ColumnDef::new(dynamic::Column::LastPlayedAt).big_integer())
                    .col(ColumnDef::new(dynamic::Column::RemoteLastPlayedAt).big_integer())
                    .col(ColumnDef::new(dynamic::Column::FavoriteAt).big_integer())
                    .col(
                        ColumnDef::new(dynamic::Column::LastPosition)
                            .big_integer()
                            .null(),
                    )
                    .col(timestamp_col(dynamic::Column::UpdateAt))
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk-dynamic-item")
                            .from(dynamic::Entity, dynamic::Column::Id)
                            .to(item::Entity, item::Column::Id)
                            .on_delete(ForeignKeyAction::Cascade)
                            .on_update(ForeignKeyAction::Cascade),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_table(
                Table::create()
                    .table(image::Entity)
                    .if_not_exists()
                    .col(
                        ColumnDef::new(image::Column::Id)
                            .big_integer()
                            .not_null()
                            .auto_increment()
                            .primary_key(),
                    )
                    .col(
                        ColumnDef::new(image::Column::ItemId)
                            .big_integer()
                            .not_null(),
                    )
                    .col(ColumnDef::new(image::Column::ImageType).string().not_null())
                    .col(ColumnDef::new(image::Column::NativeId).string())
                    .col(ColumnDef::new(image::Column::Db).string())
                    .col(ColumnDef::new(image::Column::Fresh).string().not_null())
                    .col(
                        ColumnDef::new(image::Column::Timestamp)
                            .timestamp()
                            .not_null(),
                    )
                    .foreign_key(
                        ForeignKey::create()
                            .name("fk-image-item")
                            .from(image::Entity, image::Column::ItemId)
                            .to(item::Entity, item::Column::Id)
                            .on_delete(ForeignKeyAction::Cascade)
                            .on_update(ForeignKeyAction::Cascade),
                    )
                    .index(
                        Index::create()
                            .unique()
                            .name("idx-image-unique")
                            .col(image::Column::ItemId)
                            .col(image::Column::ImageType),
                    )
                    .to_owned(),
            )
            .await?;

        manager
            .create_index(
                sea_query::Index::create()
                    .name("rel_song_artist::ArtistId")
                    .table(rel_song_artist::Entity)
                    .col(rel_song_artist::Column::ArtistId)
                    .to_owned(),
            )
            .await?;
        manager
            .create_index(
                sea_query::Index::create()
                    .name("rel_song_artist::SongId")
                    .table(rel_song_artist::Entity)
                    .col(rel_song_artist::Column::SongId)
                    .to_owned(),
            )
            .await?;

        manager
            .create_index(
                sea_query::Index::create()
                    .name("rel_album_artist::AlbumId")
                    .table(rel_album_artist::Entity)
                    .col(rel_album_artist::Column::AlbumId)
                    .to_owned(),
            )
            .await?;

        manager
            .create_index(
                sea_query::Index::create()
                    .name("rel_album_artist::ArtistId")
                    .table(rel_album_artist::Entity)
                    .col(rel_album_artist::Column::ArtistId)
                    .to_owned(),
            )
            .await?;

        Ok(())
    }

    async fn down(&self, _manager: &SchemaManager) -> Result<(), DbErr> {
        Ok(())
    }
}
