use crate::{
    api::{
        helper_sort::{album_sort_col, order_songs, song_sort_col},
        pagination::PageParams,
    },
    db::filter::SelectQcmMsgFilters,
};
use crate::{
    convert::QcmInto,
    error::ProcessError,
    msg::{
        self, GetAlbumArtistsRsp, GetAlbumsRsp, GetArtistsRsp, GetSongsRsp, MessageType, QcmMessage,
    },
};
use qcm_core::model as sqlm;
use qcm_core::model::type_enum::MixType;
use sea_orm::{
    ColumnTrait, ConnectionTrait, EntityName, EntityTrait, ModelTrait, PaginatorTrait, QueryFilter,
    QueryOrder, QuerySelect, Statement,
};
#[derive(sea_orm::FromQueryResult)]
struct EntityId {
    id: i64,
}

pub fn handles(kind: MessageType) -> bool {
    matches!(
        kind,
        MessageType::GetAlbumReq
            | MessageType::GetAlbumsReq
            | MessageType::GetAlbumArtistsReq
            | MessageType::GetArtistsReq
            | MessageType::GetArtistReq
            | MessageType::GetMixsReq
            | MessageType::GetRemoteMixsReq
            | MessageType::GetMixReq
            | MessageType::GetMixSongsReq
            | MessageType::GetSongsByIdReq
            | MessageType::GetArtistAlbumReq
            | MessageType::SearchReq
    )
}

pub async fn execute(
    db: &sea_orm::DatabaseTransaction,
    message: &QcmMessage,
) -> Result<QcmMessage, ProcessError> {
    use msg::qcm_message::Payload;
    let payload = &message.payload;
    match message.r#type() {
        MessageType::GetAlbumReq => {
            if let Some(Payload::GetAlbumReq(req)) = payload {
                if sqlm::album::Entity::find_by_id(req.id)
                    .one(db)
                    .await?
                    .is_none()
                {
                    return Err(ProcessError::NoSuchAlbum(req.id.to_string()));
                }
                let songs = order_songs(
                    sqlm::song::Entity::find().filter(sqlm::song::Column::AlbumId.eq(req.id)),
                    msg::model::SongSort::TrackNumber,
                    sea_orm::Order::Asc,
                )
                .select_only()
                .column(sqlm::song::Column::Id)
                .into_tuple::<i64>()
                .all(db)
                .await?;
                return Ok(msg::GetAlbumRsp {
                    id: req.id,
                    song_ids: songs,
                }
                .qcm_into());
            }
        }
        MessageType::GetAlbumsReq => {
            if let Some(Payload::GetAlbumsReq(req)) = payload {
                let page_params = PageParams::new(req.page, req.page_size);

                let sort: msg::model::AlbumSort =
                    req.sort.try_into().unwrap_or(msg::model::AlbumSort::Title);
                let sort_asc = req.sort_asc.qcm_into();

                let query = sqlm::album::Entity::find()
                    .inner_join(sqlm::item::Entity)
                    .left_join(sqlm::dynamic::Entity)
                    .filter(sqlm::item::Column::LibraryId.is_in(req.library_id.clone()))
                    .filter(sqlm::dynamic::Column::IsExternal.eq(false))
                    .qcm_filters(&req.filters, &req.filter_logics)
                    .order_by(album_sort_col(sort), sort_asc);

                let paginator = query
                    .select_only()
                    .column(sqlm::album::Column::Id)
                    .into_tuple::<i64>()
                    .paginate(db, page_params.page_size);

                let total = paginator.num_items().await?;
                let albums = paginator.fetch_page(page_params.page).await?;

                let ids = albums;

                let rsp = GetAlbumsRsp {
                    ids,
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetAlbumArtistsReq => {
            if let Some(Payload::GetAlbumArtistsReq(req)) = payload {
                let page_params = PageParams::new(req.page, req.page_size);

                let sort: msg::model::ArtistSort =
                    req.sort.try_into().unwrap_or(msg::model::ArtistSort::Name);
                let sort_col: sqlm::artist::Column = sort.qcm_into();
                let paginator = sqlm::artist::Entity::find()
                    .inner_join(sqlm::item::Entity)
                    .filter(sqlm::item::Column::LibraryId.is_in(req.library_id.clone()))
                    .inner_join(sqlm::rel_album_artist::Entity)
                    .qcm_filters(&req.filters, &req.filter_logics)
                    .order_by(sort_col, req.sort_asc.qcm_into())
                    .distinct()
                    .select_only()
                    .column(sqlm::artist::Column::Id)
                    .into_tuple::<i64>()
                    .paginate(db, page_params.page_size);

                let total = paginator.num_items().await?;
                let artists = paginator.fetch_page(page_params.page).await?;

                let rsp = GetAlbumArtistsRsp {
                    ids: artists,
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetArtistsReq => {
            if let Some(Payload::GetArtistsReq(req)) = payload {
                let page_params = PageParams::new(req.page, req.page_size);
                let sort: msg::model::ArtistSort =
                    req.sort.try_into().unwrap_or(msg::model::ArtistSort::Name);
                let sort_col: sqlm::artist::Column = sort.qcm_into();
                let paginator = sqlm::artist::Entity::find()
                    .inner_join(sqlm::item::Entity)
                    .filter(sqlm::item::Column::LibraryId.is_in(req.library_id.clone()))
                    .inner_join(sqlm::rel_song_artist::Entity)
                    .qcm_filters(&req.filters, &req.filter_logics)
                    .order_by(sort_col, req.sort_asc.qcm_into())
                    .distinct()
                    .select_only()
                    .column(sqlm::artist::Column::Id)
                    .into_tuple::<i64>()
                    .paginate(db, page_params.page_size);

                let total = paginator.num_items().await?;
                let artists = paginator.fetch_page(page_params.page).await?;

                let rsp = GetArtistsRsp {
                    ids: artists,
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetArtistReq => {
            if let Some(Payload::GetArtistReq(req)) = payload {
                let artist = sqlm::artist::Entity::find_by_id(req.id)
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchArtist(req.id.to_string()))?;

                let rsp = msg::GetArtistRsp { id: artist.id };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetMixsReq => {
            if let Some(Payload::GetMixsReq(req)) = payload {
                let page_params = PageParams::new(req.page, req.page_size);

                let paginator = sqlm::mix::Entity::find()
                    .filter(sqlm::mix::Column::MixType.ne(MixType::Cache))
                    .qcm_filters(&req.filters, &req.filter_logics)
                    .select_only()
                    .column(sqlm::mix::Column::Id)
                    .into_tuple::<i64>()
                    .paginate(db, page_params.page_size);

                let total = paginator.num_items().await?;
                let mixes = paginator.fetch_page(page_params.page).await?;

                let rsp = msg::GetMixsRsp {
                    ids: mixes,
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetRemoteMixsReq => {
            if let Some(Payload::GetRemoteMixsReq(req)) = payload {
                let page_params = PageParams::new(req.page, req.page_size);

                let paginator = sqlm::remote_mix::Entity::find()
                    .qcm_filters(&req.filters, &req.filter_logics)
                    .find_also_related(sqlm::mix::Entity)
                    .paginate(db, page_params.page_size);

                let total = paginator.num_items().await?;
                let mixes = paginator
                    .fetch_page(page_params.page)
                    .await?
                    .into_iter()
                    .filter_map(|(_, m)| m.map(|item| item.id))
                    .collect();

                let rsp = msg::GetRemoteMixsRsp {
                    ids: mixes,
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetMixReq => {
            if let Some(Payload::GetMixReq(req)) = payload {
                let mix = sqlm::mix::Entity::find_by_id(req.id)
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchMix(req.id.to_string()))?;

                let rsp = msg::GetMixRsp { id: mix.id };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetMixSongsReq => {
            if let Some(Payload::GetMixSongsReq(req)) = payload {
                let page_params = PageParams::new(req.page, req.page_size);

                let sort: msg::model::SongSort =
                    req.sort.try_into().unwrap_or(msg::model::SongSort::Title);
                let sort_asc = req.sort_asc.qcm_into();

                let query = sqlm::song::Entity::find()
                    .inner_join(sqlm::item::Entity)
                    .inner_join(sqlm::rel_mix_song::Entity)
                    .filter(sqlm::rel_mix_song::Column::MixId.eq(req.id))
                    .order_by(song_sort_col(sort), sort_asc)
                    .order_by(sqlm::rel_mix_song::Column::OrderIdx, sea_orm::Order::Desc);

                let paginator = query
                    .select_only()
                    .column(sqlm::song::Column::Id)
                    .into_tuple::<i64>()
                    .paginate(db, page_params.page_size);

                let total = paginator.num_items().await?;
                let songs = paginator.fetch_page(page_params.page).await?;

                let ids = songs;

                let mix = sqlm::mix::Entity::find_by_id(req.id)
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchMix(req.id.to_string()))?;

                let rsp = msg::GetMixSongsRsp {
                    mix_id: mix.id,
                    ids,
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetSongsByIdReq => {
            if let Some(Payload::GetSongsByIdReq(req)) = payload {
                if req.ids.len() > 20000 || req.ids.iter().any(|id| *id <= 0) {
                    return Err(ProcessError::WrongId("song IDs".into()));
                }
                return Ok(msg::GetSongsByIdRsp {
                    ids: req.ids.clone(),
                    total: req.ids.len() as i32,
                }
                .qcm_into());
            }
        }
        MessageType::GetArtistAlbumReq => {
            if let Some(Payload::GetArtistAlbumReq(req)) = payload {
                let page_params = PageParams::new(req.page, req.page_size);
                let sort: msg::model::AlbumSort = req
                    .sort
                    .try_into()
                    .unwrap_or(msg::model::AlbumSort::PublishTime);
                let sort_col = album_sort_col(sort);

                let artist = sqlm::artist::Entity::find_by_id(req.id)
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchArtist(req.id.to_string()))?;

                let albums_query = artist
                    .find_related(sqlm::album::Entity)
                    .order_by(sort_col, req.sort_asc.qcm_into());
                let paginator = albums_query
                    .select_only()
                    .column(sqlm::album::Column::Id)
                    .into_tuple::<i64>()
                    .paginate(db, page_params.page_size);

                let total = paginator.num_items().await?;
                let albums = paginator.fetch_page(page_params.page).await?;

                let ids = albums;

                let rsp = msg::GetArtistAlbumRsp {
                    ids,
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::SearchReq => {
            if let Some(Payload::SearchReq(req)) = payload {
                let page_params = PageParams::new(req.page, req.page_size);
                let search_query = req.query.clone();
                let mut albums_rsp = None;
                let mut songs_rsp = None;
                let mut artists_rsp = None;

                let library_ids = if search_query.is_empty() {
                    String::new()
                } else {
                    req.library_id
                        .iter()
                        .map(|id| id.to_string())
                        .collect::<Vec<_>>()
                        .join(",")
                };

                let format_query = |table: &str, fts: &str| {
                    let db_backend = db.get_database_backend();
                    let item_table_et = sqlm::item::Entity::default();
                    let item_table = item_table_et.table_name();
                    Statement::from_sql_and_values(
                        db_backend,
                        format!(
                            r#"
                                    SELECT {table}.id FROM {table}
                                    INNER JOIN {item_table} ON {item_table}.id = {table}.id
                                    INNER JOIN {fts} ON {table}.id = {fts}.rowid
                                    WHERE {fts} MATCH ('name:' || qcm_query(?)) AND {item_table}.library_id IN ({library_ids})
                                    "#
                        ),
                        [search_query.clone().into()],
                    )
                };

                for search_type in &req.types {
                    let search_type: msg::SearchType =
                        msg::SearchType::try_from(search_type.clone())
                            .map_err(|_| ProcessError::NoSuchSearchType(search_type.to_string()))?;
                    match search_type {
                        msg::SearchType::Album => {
                            let entity = sqlm::album::Entity::default();
                            let table = entity.table_name();
                            let fts = format!("{table}_fts");
                            let albums_query = sqlm::album::Entity::find()
                                .from_raw_sql(format_query(table, &fts))
                                .into_model::<EntityId>();

                            let paginator = albums_query.paginate(db, page_params.page_size);
                            let total = paginator.num_items().await?;
                            let albums = paginator.fetch_page(page_params.page).await?;

                            let ids = albums.into_iter().map(|item| item.id).collect();

                            albums_rsp = Some(GetAlbumsRsp {
                                ids,
                                total: total as i32,
                                has_more: page_params.has_more(total),
                            });
                        }
                        msg::SearchType::Song => {
                            let entity = sqlm::song::Entity::default();
                            let table = entity.table_name();
                            let fts = format!("{table}_fts");
                            let query = sqlm::song::Entity::find()
                                .from_raw_sql(format_query(table, &fts))
                                .into_model::<EntityId>();

                            let paginator = query.paginate(db, page_params.page_size);
                            let total = paginator.num_items().await?;
                            let songs = paginator.fetch_page(page_params.page).await?;

                            let ids = songs.into_iter().map(|item| item.id).collect();

                            songs_rsp = Some(GetSongsRsp {
                                ids,
                                total: total as i32,
                                has_more: page_params.has_more(total),
                            });
                        }
                        msg::SearchType::Artist => {
                            let entity = sqlm::artist::Entity::default();
                            let table = entity.table_name();
                            let fts = format!("{table}_fts");
                            let query = sqlm::artist::Entity::find()
                                .from_raw_sql(format_query(table, &fts))
                                .into_model::<EntityId>();
                            let page_params = PageParams::new(req.page, req.page_size);

                            let paginator = query.paginate(db, page_params.page_size);
                            let total = paginator.num_items().await?;

                            let artists = paginator
                                .fetch_page(page_params.page)
                                .await?
                                .into_iter()
                                .map(|a| a.id)
                                .collect();

                            artists_rsp = Some(GetArtistsRsp {
                                ids: artists,
                                total: total as i32,
                                has_more: page_params.has_more(total),
                            });
                        }
                    }
                }

                let rsp = msg::SearchRsp {
                    albums: albums_rsp,
                    artists: artists_rsp,
                    songs: songs_rsp,
                };
                return Ok(rsp.qcm_into());
            }
        }
        _ => (),
    }
    Err(ProcessError::NotFound)
}
