use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize, DeriveEntityModel)]
#[sea_orm(table_name = "program")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub id: i64,
    pub library_id: i64, // foreign
    pub native_id: String,
    pub radio_id: i64, // foreign
    #[serde(default)]
    pub sort_name: Option<String>,
    pub name: String,
    pub description: String,
    pub duration: i64,
    // pub song_id: String,
    pub serial_number: i32,
    pub create_time: DateTimeUtc,
    pub edit_time: DateTimeUtc,
}

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {
    #[sea_orm(
        belongs_to = "super::library::Entity",
        from = "Column::LibraryId",
        to = "super::library::Column::LibraryId"
    )]
    Library,
}
impl Related<super::library::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Library.def()
    }
}
impl ActiveModelBehavior for ActiveModel {}
