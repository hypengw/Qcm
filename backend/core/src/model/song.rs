use super::util::{default_json_arr, default_true};
use crate::db::values::Timestamp;
use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, DeriveEntityModel, Serialize, Deserialize)]
#[sea_orm(table_name = "song")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub id: i64,

    pub name: String,
    #[serde(default)]
    pub sort_name: Option<String>,
    #[sea_orm(nullable)]
    pub album_id: Option<i64>,
    pub track_number: i32,
    pub disc_number: i32,
    pub duration: i64,
    #[serde(default = "default_true")]
    pub can_play: bool,
    #[serde(default)]
    pub popularity: f64,
    #[serde(default = "default_json_arr")]
    pub tags: Json,

    #[serde(default)]
    pub publish_time: Option<Timestamp>,
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
    #[sea_orm(has_one = "super::dynamic::Entity")]
    Dynamic,
    #[sea_orm(
        belongs_to = "super::album::Entity",
        from = "Column::AlbumId",
        to = "super::album::Column::Id"
    )]
    Album,
    #[sea_orm(has_many = "super::image::Entity")]
    Image,
    #[sea_orm(has_many = "super::rel_mix_song::Entity")]
    RelMix,
    #[sea_orm(has_many = "super::rel_song_artist::Entity")]
    RelArtist,
}

impl Related<super::item::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Item.def()
    }
}

impl Related<super::dynamic::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Dynamic.def()
    }
}

impl Related<super::album::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Album.def()
    }
}
impl Related<super::image::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Image.def()
    }
}

impl Related<super::rel_mix_song::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::RelMix.def()
    }
}
impl Related<super::rel_song_artist::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::RelArtist.def()
    }
}

impl Related<super::artist::Entity> for Entity {
    fn to() -> RelationDef {
        super::rel_song_artist::Relation::Artist.def()
    }

    fn via() -> Option<RelationDef> {
        Some(super::rel_song_artist::Relation::Song.def().rev())
    }
}

impl Related<super::mix::Entity> for Entity {
    fn to() -> RelationDef {
        super::rel_mix_song::Relation::Mix.def()
    }

    fn via() -> Option<RelationDef> {
        Some(super::rel_mix_song::Relation::Song.def().rev())
    }
}

impl ActiveModelBehavior for ActiveModel {}
