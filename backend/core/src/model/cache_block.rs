use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, DeriveEntityModel, Serialize, Deserialize)]
#[sea_orm(table_name = "cache_block")]
pub struct Model {
    #[sea_orm(primary_key, auto_increment = false)]
    pub key: String,
    pub source_key: String,
    pub block_index: i32,
    pub block_size: i64,

    #[serde(default)]
    #[sea_orm(nullable)]
    pub blob: Option<Vec<u8>>,

    #[serde(default = "chrono::Utc::now")]
    #[sea_orm(default_expr = "Expr::current_timestamp()")]
    pub timestamp: DateTimeUtc,
}

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {
    #[sea_orm(
        belongs_to = "super::cache_source::Entity",
        from = "Column::SourceKey",
        to = "super::cache_source::Column::Key"
    )]
    Source,
}

impl Related<super::cache_source::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Source.def()
    }
}

impl ActiveModelBehavior for ActiveModel {}

pub async fn exists(db: &DatabaseConnection, block_key: &str) -> bool {
    match Entity::find_by_id(block_key.to_string()).one(db).await {
        Ok(Some(_)) => true,
        _ => false,
    }
}

pub async fn query_by_key(db: &DatabaseConnection, block_key: &str) -> Option<Model> {
    match Entity::find_by_id(block_key.to_string()).one(db).await {
        Ok(info) => info,
        Err(e) => {
            log::error!("{:?}", e);
            None
        }
    }
}
