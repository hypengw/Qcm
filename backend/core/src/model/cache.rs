use super::type_enum::CacheType;
use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, DeriveEntityModel, Serialize, Deserialize)]
#[sea_orm(table_name = "cache")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub id: i64,
    #[sea_orm(unique)]
    pub key: String,
    pub cache_type: CacheType,

    pub content_type: String,
    // u64 is not supported by sqlite
    pub content_length: i64,

    #[serde(default)]
    #[sea_orm(nullable)]
    pub blob: Option<Vec<u8>>,

    #[serde(default = "chrono::Utc::now")]
    #[sea_orm(default_expr = "Expr::current_timestamp()")]
    pub timestamp: DateTimeUtc,

    #[serde(default = "chrono::Utc::now")]
    #[sea_orm(default_expr = "Expr::current_timestamp()")]
    pub last_use: DateTimeUtc,
}

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {}

impl ActiveModelBehavior for ActiveModel {}

pub async fn blob_chunk(
    db: &DatabaseConnection,
    id: Option<i64>,
    key: Option<String>,
    offset: u64,
    chunk_size: u64,
) -> Result<bytes::Bytes, sea_orm::DbErr> {
    use sea_orm::QuerySelect;
    let expr = match (id, key) {
        (Some(id), _) => Column::Id.eq(id),
        (None, Some(key)) => Column::Key.eq(key),
        (None, None) => return Err(DbErr::Custom("query cache without filter".to_string())),
    };
    let chunk: Option<String> = Entity::find()
        .filter(expr)
        .select_only()
        .column_as(
            Expr::cust_with_values(
                "substr(blob, ?, ?)",
                [
                    Value::BigUnsigned(Some(offset + 1)),
                    Value::BigUnsigned(Some(chunk_size)),
                ],
            ),
            "chunk",
        )
        .into_tuple()
        .one(db)
        .await?;
    chunk
        .map(|s| {
            use bytes::BufMut;
            let mut buf = bytes::BytesMut::new();
            buf.put(s.as_bytes());
            buf.freeze()
        })
        .ok_or(DbErr::Custom(String::new()))
}

pub async fn query_by_key(db: &DatabaseConnection, key: &str) -> Option<Model> {
    use sea_orm::ColumnTrait;
    match Entity::find()
        .filter(Column::Key.eq(key.to_string()))
        .one(db)
        .await
    {
        Ok(info) => info,
        Err(e) => {
            log::error!("{:?}", e);
            None
        }
    }
}
