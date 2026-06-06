use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize, DeriveEntityModel)]
#[sea_orm(table_name = "remote_mix")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub id: i64,
    pub name: String,
    #[serde(default)]
    pub description: Option<String>,
    pub track_count: i32,
    pub mix_type: String,
}

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {
    #[sea_orm(
        belongs_to = "super::item::Entity",
        from = "Column::Id",
        to = "super::item::Column::Id"
    )]
    Item,
    #[sea_orm(has_one = "super::mix::Entity")]
    Mix,
    #[sea_orm(has_many = "super::image::Entity")]
    Image,
}
impl Related<super::item::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Item.def()
    }
}

impl Related<super::mix::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Mix.def()
    }
}

impl Related<super::image::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Image.def()
    }
}

impl Related<super::song::Entity> for Entity {
    fn to() -> RelationDef {
        super::rel_mix_song::Relation::Song.def()
    }

    fn via() -> Option<RelationDef> {
        Some(super::rel_mix_song::Relation::Mix.def().rev())
    }
}

impl ActiveModelBehavior for ActiveModel {}
