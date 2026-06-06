use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, DeriveEntityModel, Serialize, Deserialize)]
#[sea_orm(table_name = "cache")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub id: i64,
    #[sea_orm(unique)]
    pub key: String,
    pub order: i32,
    pub parent_key: String,
}

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {
    #[sea_orm(
        belongs_to = "super::cache::Entity",
        from = "Column::Key",
        to = "super::cache::Column::Key"
    )]
    #[sea_orm(
        belongs_to = "super::cache::Entity",
        from = "Column::ParentKey",
        to = "super::cache::Column::Key"
    )]
    CacheKey,
}

impl Related<super::cache::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::CacheKey.def()
    }
}

impl ActiveModelBehavior for ActiveModel {}
