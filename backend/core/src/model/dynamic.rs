use crate::db::values::Timestamp;
use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Default, PartialEq, DeriveEntityModel, Serialize, Deserialize)]
#[sea_orm(table_name = "dynamic")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub id: i64,

    #[serde(default)]
    #[sea_orm(default)]
    pub is_external: bool,

    #[serde(default)]
    #[sea_orm(default)]
    pub play_count: i64,

    #[serde(default)]
    #[sea_orm(default)]
    pub remote_play_count: i64,

    #[serde(default)]
    #[sea_orm(default, nullable)]
    pub last_position: Option<i64>,

    #[serde(default)]
    pub last_played_at: Option<Timestamp>,

    #[serde(default)]
    pub remote_last_played_at: Option<Timestamp>,

    #[serde(default)]
    pub favorite_at: Option<Timestamp>,

    #[serde(default = "Timestamp::now")]
    #[sea_orm(default_expr = "Timestamp::now_expr()")]
    pub update_at: Timestamp,
}

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {
    #[sea_orm(
        belongs_to = "super::item::Entity",
        from = "Column::Id",
        to = "super::item::Column::Id"
    )]
    Item,
    #[sea_orm(
        belongs_to = "super::song::Entity",
        from = "Column::Id",
        to = "super::song::Column::Id"
    )]
    Song,
    #[sea_orm(
        belongs_to = "super::album::Entity",
        from = "Column::Id",
        to = "super::album::Column::Id"
    )]
    Album,
}

impl Related<super::item::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Item.def()
    }
}
impl Related<super::album::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Album.def()
    }
}
impl Related<super::song::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Song.def()
    }
}

impl ActiveModelBehavior for ActiveModel {}
