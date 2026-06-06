use super::type_enum::AlbumType;
use crate::db::values::Timestamp;
use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, DeriveEntityModel, Serialize, Deserialize)]
#[sea_orm(table_name = "album")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub id: i64,

    pub name: String,
    #[serde(default)]
    pub sort_name: Option<String>,
    pub track_count: i32,
    pub disc_count: i32,

    #[serde(default)]
    pub r#type: AlbumType,
    #[serde(default)]
    pub duration: i64,
    #[serde(default)]
    pub language: Option<String>,
    #[serde(default)]
    pub description: Option<String>,
    #[serde(default)]
    pub company: Option<String>,

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
    #[sea_orm(has_many = "super::song::Entity")]
    Song,
    #[sea_orm(has_many = "super::image::Entity")]
    Image,
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

impl Related<super::song::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Song.def()
    }
}

impl Related<super::image::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Image.def()
    }
}

impl Related<super::artist::Entity> for Entity {
    fn to() -> RelationDef {
        super::rel_album_artist::Relation::Artist.def()
    }

    fn via() -> Option<RelationDef> {
        Some(super::rel_album_artist::Relation::Album.def().rev())
    }
}

impl ActiveModelBehavior for ActiveModel {}
