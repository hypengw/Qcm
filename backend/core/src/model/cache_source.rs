use super::type_enum::CacheType;
use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, DeriveEntityModel, Serialize, Deserialize)]
#[sea_orm(table_name = "cache_source")]
pub struct Model {
    #[sea_orm(primary_key, auto_increment = false)]
    pub key: String,
    pub cache_type: CacheType,
    pub content_type: String,
    pub content_length: i64,
    pub block_count: i32,

    #[serde(default = "chrono::Utc::now")]
    #[sea_orm(default_expr = "Expr::current_timestamp()")]
    pub timestamp: DateTimeUtc,

    #[serde(default = "chrono::Utc::now")]
    #[sea_orm(default_expr = "Expr::current_timestamp()")]
    pub last_use: DateTimeUtc,
}

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {
    #[sea_orm(has_many = "super::cache_block::Entity")]
    Blocks,
}

impl Related<super::cache_block::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Blocks.def()
    }
}

impl ActiveModelBehavior for ActiveModel {}

pub async fn query_by_key(db: &DatabaseConnection, key: &str) -> Option<Model> {
    match Entity::find_by_id(key.to_string()).one(db).await {
        Ok(info) => info,
        Err(e) => {
            log::error!("{:?}", e);
            None
        }
    }
}
