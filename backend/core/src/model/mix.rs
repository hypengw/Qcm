use crate::db::values::Timestamp;
use crate::db::DbOper;
use crate::model::{self as sqlm, type_enum::MixType};
use sea_orm::DbErr;
use sea_orm::{entity::prelude::*, ConnectionTrait, DatabaseTransaction};
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize, DeriveEntityModel)]
#[sea_orm(table_name = "mix")]
pub struct Model {
    #[sea_orm(primary_key)]
    pub id: i64,
    pub name: String,
    pub track_count: i32,
    pub mix_type: MixType,
    #[serde(default)]
    pub sort_name: Option<String>,
    #[serde(default)]
    pub description: String,
    #[serde(default)]
    pub remote_id: Option<i64>,

    #[serde(default = "Timestamp::now")]
    #[sea_orm(default_expr = "Timestamp::now_expr()")]
    pub create_at: Timestamp,

    #[serde(default = "Timestamp::now")]
    #[sea_orm(default_expr = "Timestamp::now_expr()")]
    pub update_at: Timestamp,

    #[serde(default)]
    #[sea_orm(default_expr = "Timestamp::default_expr()")]
    pub content_update_at: Timestamp,
}

#[derive(Copy, Clone, Debug, EnumIter, DeriveRelation)]
pub enum Relation {
    #[sea_orm(has_many = "super::rel_mix_song::Entity")]
    RelSong,
    #[sea_orm(
        belongs_to = "super::remote_mix::Entity",
        from = "Column::RemoteId",
        to = "super::remote_mix::Column::Id"
    )]
    Remote,
}

impl Related<super::rel_mix_song::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::RelSong.def()
    }
}

impl Related<super::remote_mix::Entity> for Entity {
    fn to() -> RelationDef {
        Relation::Remote.def()
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

pub async fn reorder_append(db: &DatabaseTransaction, mix_id: i64) -> Result<(), DbErr> {
    use sea_orm::Statement;
    db.execute(Statement::from_string(
                            db.get_database_backend(),
                            format!("
                                WITH 
                                    max_order AS (
                                        SELECT COALESCE(MAX(order_idx), -1) AS max_order_idx
                                        FROM rel_mix_song
                                        WHERE mix_id = {id}
                                    ),
                                    new_orders AS (
                                        SELECT song_id, max_order.max_order_idx + ROW_NUMBER() OVER (ORDER BY id) AS new_order_idx
                                        FROM rel_mix_song
                                        JOIN max_order
                                        WHERE mix_id = {id} AND order_idx = 0 
                                    )
                                UPDATE rel_mix_song
                                SET order_idx = (SELECT new_order_idx FROM new_orders WHERE rel_mix_song.song_id = new_orders.song_id)
                                WHERE mix_id = {id} AND order_idx = 0
                            ", id = mix_id),
                        ))
                        .await?;
    Ok(())
}

pub async fn append_songs(
    db: &DatabaseTransaction,
    mix_id: i64,
    song_ids: &Vec<i64>,
) -> Result<u64, DbErr> {
    let models = song_ids
        .iter()
        .map(|song_id| sqlm::rel_mix_song::ActiveModel {
            mix_id: sea_orm::Set(mix_id),
            song_id: sea_orm::Set(*song_id),
            order_idx: sea_orm::Set(0),
            ..Default::default()
        });

    DbOper::insert(
        &db,
        models,
        &[
            sqlm::rel_mix_song::Column::SongId,
            sqlm::rel_mix_song::Column::MixId,
        ],
        &[sqlm::rel_mix_song::Column::OrderIdx],
    )
    .await?;

    let count = sqlm::rel_mix_song::Entity::find()
        .filter(sqlm::rel_mix_song::Column::MixId.eq(mix_id))
        .filter(sqlm::rel_mix_song::Column::OrderIdx.eq(0))
        .count(db)
        .await?;

    reorder_append(&db, mix_id).await?;

    Entity::update_many()
        .col_expr(
            Column::TrackCount,
            Expr::val(count).add(Expr::col(Column::TrackCount)).into(),
        )
        .exec(db)
        .await?;

    Ok(count)
}
