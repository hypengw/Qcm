use super::basic::QueryBuilder;
use super::DbChunkOper;
use crate::db::values::Timestamp;
use crate::model::{self as sqlm};
use sea_orm::sea_query::OnConflict;
use sea_orm::{prelude::DateTimeUtc, DatabaseTransaction, EntityTrait};
use sea_orm::{prelude::*, QuerySelect, Statement};
use sea_orm::{sea_query, sea_query::Alias, Condition};

pub async fn sync_drop_before(
    txn: &DatabaseTransaction,
    provider_id: i64,
    now: DateTimeUtc,
) -> Result<(), sea_orm::DbErr> {
    use sea_query::{CommonTableExpression, Expr, Query, WithClause};

    let now_ts: Timestamp = now.into();
    // clean library first
    sqlm::library::Entity::delete_many()
        .filter(sqlm::library::Column::ProviderId.eq(provider_id))
        .filter(sqlm::library::Column::EditTime.lt(now))
        .exec(txn)
        .await?;

    let ids: Vec<i64> = sqlm::library::Entity::find()
        .select_only()
        .column(sqlm::library::Column::LibraryId)
        .filter(sqlm::library::Column::ProviderId.eq(provider_id))
        .into_tuple()
        .all(txn)
        .await?;

    {
        let cte = CommonTableExpression::new()
            .query(
                Query::select()
                    .column(sqlm::rel_album_artist::Column::AlbumId)
                    .from(sqlm::rel_album_artist::Entity)
                    .inner_join(
                        sqlm::item::Entity,
                        Condition::all()
                            .add(
                                Expr::col((
                                    sqlm::rel_album_artist::Entity,
                                    sqlm::rel_album_artist::Column::AlbumId,
                                ))
                                .equals((sqlm::item::Entity, sqlm::item::Column::Id)),
                            )
                            .add(
                                Expr::col((sqlm::item::Entity, sqlm::item::Column::LibraryId))
                                    .is_in(ids.clone()),
                            ),
                    )
                    .and_where(
                        Expr::col((
                            sqlm::rel_album_artist::Entity,
                            sqlm::rel_album_artist::Column::UpdateAt,
                        ))
                        .lt(now_ts),
                    )
                    .to_owned(),
            )
            .columns([sqlm::rel_album_artist::Column::AlbumId])
            .table_name("to_delete")
            .to_owned();

        let with = WithClause::new().cte(cte).to_owned();

        let delete = Query::delete()
            .from_table(sqlm::rel_album_artist::Entity)
            .and_where(
                Expr::col((
                    sqlm::rel_album_artist::Entity,
                    sqlm::rel_album_artist::Column::AlbumId,
                ))
                .in_subquery(
                    Query::select()
                        .column(sqlm::rel_album_artist::Column::AlbumId)
                        .from("to_delete")
                        .to_owned(),
                ),
            )
            .to_owned();

        let query = delete.with(with);

        let stmt = query.build(QueryBuilder);

        txn.execute(Statement::from_sql_and_values(
            txn.get_database_backend(),
            stmt.0,
            stmt.1,
        ))
        .await?;
    }

    {
        let cte = CommonTableExpression::new()
            .query(
                Query::select()
                    .column(sqlm::rel_song_artist::Column::SongId)
                    .from(sqlm::rel_song_artist::Entity)
                    .inner_join(
                        sqlm::item::Entity,
                        Condition::all()
                            .add(
                                Expr::col((
                                    sqlm::rel_song_artist::Entity,
                                    sqlm::rel_song_artist::Column::SongId,
                                ))
                                .equals((sqlm::item::Entity, sqlm::item::Column::Id)),
                            )
                            .add(
                                Expr::col((sqlm::item::Entity, sqlm::item::Column::LibraryId))
                                    .is_in(ids.clone()),
                            ),
                    )
                    .and_where(
                        Expr::col((
                            sqlm::rel_song_artist::Entity,
                            sqlm::rel_song_artist::Column::UpdateAt,
                        ))
                        .lt(now_ts),
                    )
                    .to_owned(),
            )
            .columns([sqlm::rel_song_artist::Column::SongId])
            .table_name("to_delete")
            .to_owned();

        let with = WithClause::new().cte(cte).to_owned();

        let delete = Query::delete()
            .from_table(sqlm::rel_song_artist::Entity)
            .and_where(
                Expr::col((
                    sqlm::rel_song_artist::Entity,
                    sqlm::rel_song_artist::Column::SongId,
                ))
                .in_subquery(
                    Query::select()
                        .column(sqlm::rel_song_artist::Column::SongId)
                        .from("to_delete")
                        .to_owned(),
                ),
            )
            .to_owned();

        let query = delete.with(with);

        let stmt = query.build(QueryBuilder);

        txn.execute(Statement::from_sql_and_values(
            txn.get_database_backend(),
            stmt.0,
            stmt.1,
        ))
        .await?;
    }

    sqlm::item::Entity::delete_many()
        .filter(sqlm::item::Column::LastSyncAt.lt(now_ts))
        .filter(sqlm::item::Column::ProviderId.eq(provider_id))
        .exec(txn)
        .await?;

    Ok(())
}

fn select_id_from_native_id_map(
    library_id: i64,
    ids: Vec<(String, String)>,
    type1: sqlm::type_enum::ItemType,
    type2: sqlm::type_enum::ItemType,
) -> (sea_query::WithClause, sea_query::SelectStatement) {
    use sea_query::{Asterisk, CommonTableExpression, Expr, Query, WithClause};

    let input_alias = Alias::new("native_id_map");
    let input_col1 = Alias::new("input_col1");
    let input_col2 = Alias::new("input_col2");
    let with_clause = WithClause::new()
        .cte(
            CommonTableExpression::new()
                .query(
                    Query::select()
                        .column(Asterisk)
                        .from_values(ids, Alias::new("input_ids"))
                        .to_owned(),
                )
                .columns([input_col1.clone(), input_col2.clone()])
                .table_name(input_alias.clone())
                .to_owned(),
        )
        .to_owned();

    let artist_item_alias = Alias::new("artist_item");
    let now = Timestamp::now();
    let relations = sea_query::Query::select()
        .expr(Expr::col((sqlm::item::Entity, sqlm::item::Column::Id)))
        .expr(Expr::col((
            artist_item_alias.clone(),
            sqlm::item::Column::Id,
        )))
        .expr(Expr::value(now))
        .from(sqlm::item::Entity)
        .inner_join(
            input_alias.clone(),
            Expr::col((sqlm::item::Entity, sqlm::item::Column::NativeId))
                .equals((input_alias.clone(), input_col1.clone())),
        )
        .join_as(
            sea_orm::JoinType::InnerJoin,
            sqlm::item::Entity,
            artist_item_alias.clone(),
            Condition::all()
                .add(
                    Expr::col((artist_item_alias.clone(), sqlm::item::Column::LibraryId))
                        .eq(library_id),
                )
                .add(Expr::col((artist_item_alias.clone(), sqlm::item::Column::Type)).eq(type2))
                .add(
                    Expr::col((artist_item_alias.clone(), sqlm::item::Column::NativeId))
                        .equals((input_alias.clone(), input_col2.clone())),
                ),
        )
        .and_where(Expr::col((sqlm::item::Entity, sqlm::item::Column::LibraryId)).eq(library_id))
        .and_where(Expr::col((sqlm::item::Entity, sqlm::item::Column::Type)).eq(type1))
        .to_owned();
    (with_clause, relations)
}

pub async fn sync_song_artist_ids(
    txn: &DatabaseTransaction,
    library_id: i64,
    ids: Vec<(String, String)>,
) -> Result<(), sea_orm::DbErr> {
    if ids.is_empty() {
        return Ok(());
    }

    let conflict = [
        sqlm::rel_song_artist::Column::SongId,
        sqlm::rel_song_artist::Column::ArtistId,
    ];

    let (with_clause, relations) = select_id_from_native_id_map(
        library_id,
        ids,
        sqlm::type_enum::ItemType::Song,
        sqlm::type_enum::ItemType::Artist,
    );

    let stmt = sea_query::Query::insert()
        .into_table(sqlm::rel_song_artist::Entity)
        .columns([
            sqlm::rel_song_artist::Column::SongId,
            sqlm::rel_song_artist::Column::ArtistId,
            sqlm::rel_song_artist::Column::UpdateAt,
        ])
        .select_from(relations)
        .unwrap()
        .on_conflict(
            sea_query::OnConflict::columns(conflict)
                .update_column(sqlm::rel_song_artist::Column::UpdateAt)
                .to_owned(),
        )
        .to_owned()
        .with(with_clause)
        .to_owned();

    let builder = txn.get_database_backend();
    txn.execute(builder.build(&stmt)).await?;

    Ok(())
}

pub async fn sync_album_artist_ids(
    txn: &DatabaseTransaction,
    library_id: i64,
    ids: Vec<(String, String)>,
) -> Result<(), sea_orm::DbErr> {
    if ids.is_empty() {
        return Ok(());
    }

    let conflict = [
        sqlm::rel_album_artist::Column::AlbumId,
        sqlm::rel_album_artist::Column::ArtistId,
    ];

    let (with_clause, relations) = select_id_from_native_id_map(
        library_id,
        ids,
        sqlm::type_enum::ItemType::Album,
        sqlm::type_enum::ItemType::Artist,
    );

    let stmt = sea_query::Query::insert()
        .into_table(sqlm::rel_album_artist::Entity)
        .columns([
            sqlm::rel_album_artist::Column::AlbumId,
            sqlm::rel_album_artist::Column::ArtistId,
            sqlm::rel_album_artist::Column::UpdateAt,
        ])
        .select_from(relations)
        .unwrap()
        .on_conflict(
            sea_query::OnConflict::columns(conflict)
                .update_column(sqlm::rel_album_artist::Column::UpdateAt)
                .to_owned(),
        )
        .to_owned()
        .with(with_clause)
        .to_owned();
    let builder = txn.get_database_backend();
    txn.execute(builder.build(&stmt)).await?;

    Ok(())
}

pub async fn sync_song_album_ids(
    txn: &DatabaseTransaction,
    library_id: i64,
    ids: Vec<(String, String)>,
) -> Result<(), sea_orm::DbErr> {
    if ids.is_empty() {
        return Ok(());
    }

    let (mut with_clause, relations) = select_id_from_native_id_map(
        library_id,
        ids,
        sqlm::type_enum::ItemType::Song,
        sqlm::type_enum::ItemType::Album,
    );
    use sea_query::{CommonTableExpression, Expr};

    let with_clause_final = with_clause
        .cte(
            CommonTableExpression::new()
                .query(relations)
                .columns([
                    Alias::new("id"),
                    Alias::new("album_id"),
                    Alias::new("update_at"),
                ])
                .table_name(Alias::new("res"))
                .to_owned(),
        )
        .to_owned();

    let builder = txn.get_database_backend();
    let stmt = sea_query::Query::update()
        .table(sqlm::song::Entity)
        .value(
            sqlm::song::Column::AlbumId,
            Expr::col((Alias::new("res"), Alias::new("album_id"))),
        )
        .to_owned()
        .with(with_clause_final)
        .to_owned();

    let raw = format!(
        "
                {}
                FROM res
                WHERE res.id = song.id
                ",
        builder.build(&stmt).to_string()
    );
    txn.execute(Statement::from_string(builder, raw)).await?;
    Ok(())
}

pub async fn allocate_items<I>(txn: &DatabaseTransaction, items: I) -> Result<Vec<i64>, DbErr>
where
    I: IntoIterator<Item = sqlm::item::ActiveModel>,
{
    let conflict = OnConflict::new()
        .exprs([
            Expr::col(sqlm::item::Column::NativeId),
            Expr::col(sqlm::item::Column::ProviderId),
            Expr::col(sqlm::item::Column::Type),
            // use raw as sea_orm will switch to '?' for Expr::val(-1) causing issue with matching index
            Expr::expr(Expr::cust("COALESCE(library_id, -1)")),
        ])
        .update_columns([
            sqlm::item::Column::CreateAt,
            sqlm::item::Column::LastSyncAt,
            sqlm::item::Column::UpdateAt,
        ])
        .to_owned();

    let out = DbChunkOper::<50>::insert_on_return_key(txn, items, conflict).await?;
    Ok(out)
}

pub async fn check_cache_mix<DB>(
    db: &DB,
    id: i64,
) -> Result<Option<(sqlm::remote_mix::Model, sqlm::item::Model)>, DbErr>
where
    DB: sea_orm::ConnectionTrait,
{
    let out = sqlm::remote_mix::Entity::find()
        .inner_join(sqlm::mix::Entity)
        .find_also_related(sqlm::item::Entity)
        .filter(Expr::col((sqlm::mix::Entity, sqlm::mix::Column::Id)).eq(id))
        .filter(
            Expr::col((sqlm::mix::Entity, sqlm::mix::Column::ContentUpdateAt))
                .lte(Timestamp::now().as_millis() - 30 * 60 * 1000),
        )
        .one(db)
        .await?;

    if let Some((mix, Some(item))) = out {
        Ok(Some((mix, item)))
    } else {
        Ok(None)
    }
}
