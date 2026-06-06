use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, DeriveEntityModel, Serialize, Deserialize)]
#[sea_orm(table_name = "provider")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub provider_id: i64,
    pub name: String,
    #[sea_orm(column_name = "type")]
    pub type_: String,
    pub base_url: String,
    pub auth_method: Option<Json>,
    pub cookie: String,
    pub custom: String,
    #[sea_orm(default_expr = "Expr::current_timestamp()")]
    pub edit_time: DateTimeUtc,
}

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {
    #[sea_orm(has_many = "super::library::Entity")]
    Library,
}

impl Related<super::library::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Library.def()
    }
}
impl ActiveModelBehavior for ActiveModel {}
