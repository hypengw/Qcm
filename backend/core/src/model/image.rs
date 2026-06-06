use super::type_enum::ImageType;
use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, DeriveEntityModel, Serialize, Deserialize)]
#[sea_orm(table_name = "image")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub id: i64,
    pub item_id: i64,
    pub image_type: ImageType,

    #[serde(default)]
    pub native_id: Option<String>,

    #[serde(default)]
    pub db: Option<String>,

    #[serde(default)]
    pub fresh: String, // custom string for testing changed
    #[serde(default = "chrono::Utc::now")]
    pub timestamp: DateTimeUtc,
}
// unique: (item_id, image_type)

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {
    #[sea_orm(
        belongs_to = "super::item::Entity",
        from = "Column::ItemId",
        to = "super::item::Column::Id"
    )]
    Item,
    #[sea_orm(
        belongs_to = "super::album::Entity",
        from = "Column::ItemId",
        to = "super::album::Column::Id"
    )]
    Album,
    #[sea_orm(
        belongs_to = "super::song::Entity",
        from = "Column::ItemId",
        to = "super::song::Column::Id"
    )]
    Song,
    #[sea_orm(
        belongs_to = "super::artist::Entity",
        from = "Column::ItemId",
        to = "super::artist::Column::Id"
    )]
    Artist,
    #[sea_orm(
        belongs_to = "super::remote_mix::Entity",
        from = "Column::ItemId",
        to = "super::remote_mix::Column::Id"
    )]
    RemoteMix,
}

impl Related<super::item::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Item.def()
    }
}

impl Related<super::artist::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Artist.def()
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

impl Related<super::remote_mix::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::RemoteMix.def()
    }
}

impl ActiveModelBehavior for ActiveModel {}
