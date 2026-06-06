use crate::db::values::Timestamp;
use serde::{Deserialize, Serialize};

use sea_orm::entity::prelude::*;

#[derive(Clone, Debug, PartialEq, DeriveEntityModel, Serialize, Deserialize)]
#[sea_orm(table_name = "artist")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub id: i64,

    pub name: String,
    #[serde(default)]
    pub sort_name: Option<String>,
    #[serde(default)]
    pub description: String,
    #[serde(default)]
    pub album_count: i32,
    #[serde(default)]
    pub music_count: i32,
    #[serde(default)]
    pub added_at: Option<Timestamp>,
}

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {
    #[sea_orm(
        belongs_to = "super::item::Entity",
        from = "Column::Id",
        to = "super::item::Column::Id"
    )]
    Item,
    #[sea_orm(has_many = "super::image::Entity")]
    Image,
    #[sea_orm(has_many = "super::rel_album_artist::Entity")]
    RelAlbum,
    #[sea_orm(has_many = "super::rel_song_artist::Entity")]
    RelSong,
}


impl Related<super::item::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Item.def()
    }
}
impl Related<super::image::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Image.def()
    }
}
impl Related<super::rel_album_artist::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::RelAlbum.def()
    }
}
impl Related<super::rel_song_artist::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::RelSong.def()
    }
}
impl Related<super::album::Entity> for Entity {
    fn to() -> RelationDef {
        super::rel_album_artist::Relation::Album.def()
    }

    fn via() -> Option<RelationDef> {
        Some(super::rel_album_artist::Relation::Artist.def().rev())
    }
}

impl Related<super::song::Entity> for Entity {
    fn to() -> RelationDef {
        super::rel_song_artist::Relation::Song.def()
    }

    fn via() -> Option<RelationDef> {
        Some(super::rel_song_artist::Relation::Artist.def().rev())
    }
}

impl ActiveModelBehavior for ActiveModel {}
