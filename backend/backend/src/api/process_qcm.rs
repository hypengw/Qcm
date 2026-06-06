use prost::{self, Message};
use qcm_core::db::values::Timestamp;
use qcm_core::model::type_enum::MixType;
use qcm_core::provider::AuthResult;
use qcm_core::{event::Event as CoreEvent, global, Result};
use sea_orm::TransactionTrait;
use std::sync::Arc;
use tokio::sync::oneshot;

use qcm_core::model::{self as sqlm};
use sea_orm::{
    prelude::Expr, sea_query, ColumnTrait, ConnectionTrait, EntityName, EntityTrait, ModelTrait,
    PaginatorTrait, QueryFilter, QueryOrder, QuerySelect, RelationTrait, Statement,
};

use crate::api::{
    helper_extra::{extra_insert_artists, extra_insert_dynamic, to_rsp_albums, to_rsp_songs},
    helper_sort::{album_sort_col, song_sort_col},
    pagination::PageParams,
};
use crate::convert::QcmInto;
use crate::db::filter::SelectQcmMsgFilters;
use crate::error::ProcessError;
use crate::event::{ServiceContext, BackendEvent};
use crate::msg::{
    self, AuthProviderRsp, GetAlbumArtistsRsp, GetAlbumsRsp, GetArtistsRsp, GetProviderMetasRsp,
    GetSongsRsp, GetStorageInfoRsp, MessageType, QcmMessage, QrAuthUrlRsp, Rsp, SyncRsp, TestRsp,
};
use sea_orm::DatabaseConnection;

pub async fn process_qcm(
    ctx: &Arc<ServiceContext>,
    data: &[u8],
    in_id: &mut Option<i32>,
) -> Result<QcmMessage, ProcessError> {
    use msg::qcm_message::Payload;
    let message = QcmMessage::decode(data)?;
    let mtype = message.r#type();
    let payload = &message.payload;
    *in_id = Some(message.id);

    match mtype {
        MessageType::GetProviderMetasReq => {
            let response = GetProviderMetasRsp {
                metas: qcm_core::global::with_provider_metas(|metas| {
                    metas
                        .values()
                        .map(|el| -> msg::model::ProviderMeta { el.clone().qcm_into() })
                        .collect()
                }),
            };
            return Ok(response.qcm_into());
        }
        MessageType::DeleteProviderReq => {
            if let Some(Payload::DeleteProviderReq(req)) = payload {
                let id = req.provider_id;
                sqlm::provider::Entity::delete_by_id(id)
                    .exec(&ctx.provider_context.db)
                    .await?;
                global::remove_provider(id);
                let _ = ctx
                    .backend_ev
                    .send(BackendEvent::DeleteProvider { id })
                    .await;
                let rsp = Rsp::default();
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::AuthProviderReq => {
            if let Some(Payload::AuthProviderReq(req)) = payload {
                let provider = global::get_tmp_provider(&req.tmp_provider)
                    .ok_or(ProcessError::NoSuchProvider(req.tmp_provider.clone()))?;

                let mut rsp = AuthProviderRsp::default();
                let info = req
                    .auth_info
                    .clone()
                    .ok_or(ProcessError::NotFound)?
                    .qcm_into();
                match provider.auth(&ctx.provider_context, &info).await? {
                    AuthResult::Ok => {
                        rsp.code = msg::model::AuthResult::Ok.into();
                    }
                    e => {
                        rsp = e.qcm_into();
                    }
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::AddProviderReq => {
            if let Some(Payload::AddProviderReq(req)) = payload {
                let provider = global::get_tmp_provider(&req.tmp_provider)
                    .ok_or(ProcessError::NoSuchProvider(req.tmp_provider.clone()))?;
                provider.set_name(&req.name);
                let id =
                    crate::db::add_provider(&ctx.provider_context.db, provider.clone()).await?;
                provider.set_id(Some(id));

                global::add_provider(provider.clone());
                ctx.backend_ev
                    .send(BackendEvent::NewProvider { id })
                    .await?;

                return Ok(Rsp::default().qcm_into());
            }
        }
        MessageType::ReplaceProviderReq => {
            if let Some(Payload::ReplaceProviderReq(req)) = payload {
                let _provider = global::provider(req.provider_id)
                    .ok_or(ProcessError::NoSuchProvider(req.provider_id.to_string()))?;
                let tmp_provider = global::get_tmp_provider(&req.tmp_provider)
                    .ok_or(ProcessError::NoSuchProvider(req.tmp_provider.clone()))?;

                tmp_provider.set_id(Some(req.provider_id));
                // maybe: try use tmp self?
                tmp_provider.set_name(&_provider.name());

                // replace
                crate::db::add_provider(&ctx.provider_context.db, tmp_provider.clone()).await?;
                global::add_provider(tmp_provider.clone());
                return Ok(Rsp::default().qcm_into());
            }
        }
        MessageType::UpdateProviderReq => {
            if let Some(Payload::UpdateProviderReq(req)) = payload {
                let provider = global::provider(req.provider_id)
                    .ok_or(ProcessError::NoSuchProvider(req.provider_id.to_string()))?;

                let mut rsp = msg::UpdateProviderRsp::default();
                rsp.code = msg::model::AuthResult::Ok.into();
                match &req.auth_info {
                    // do replace
                    Some(auth_info) => {
                        let type_name = provider.type_name();
                        let meta = global::provider_meta(&type_name)
                            .ok_or(ProcessError::NoSuchProviderType(type_name))?;
                        let new_provider =
                            (meta.creator)(provider.id(), &req.name, &global::device_id())?;

                        let info = auth_info.clone().qcm_into();
                        match new_provider.auth(&ctx.provider_context, &info).await? {
                            AuthResult::Ok => {
                                crate::db::add_provider(
                                    &ctx.provider_context.db,
                                    new_provider.clone(),
                                )
                                .await?;
                                global::add_provider(new_provider);
                            }
                            e => {
                                rsp = e.qcm_into();
                            }
                        }
                    }
                    // do update
                    None => {
                        provider.set_name(&req.name);
                        crate::db::add_provider(&ctx.provider_context.db, provider).await?;
                    }
                }
                let _ = ctx
                    .backend_ev
                    .send(BackendEvent::UpdateProvider {
                        id: req.provider_id,
                    })
                    .await;
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::CreateTmpProviderReq => {
            if let Some(Payload::CreateTmpProviderReq(req)) = payload {
                let meta = global::provider_meta(&req.type_name)
                    .ok_or(ProcessError::NoSuchProviderType(req.type_name.clone()))?;
                let provider = (meta.creator)(None, "", &global::device_id())?;
                let key = uuid::Uuid::new_v4().to_string();
                global::set_tmp_provider(&key, provider.clone());

                let rsp = msg::CreateTmpProviderRsp { key: key };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::DeleteTmpProviderReq => {
            if let Some(Payload::DeleteTmpProviderReq(req)) = payload {
                global::remove_tmp_provider(&req.key);
            }
        }
        MessageType::QrAuthUrlReq => {
            if let Some(Payload::QrAuthUrlReq(req)) = payload {
                let provider = {
                    match global::get_tmp_provider(&req.tmp_provider) {
                        Some(tmp) => tmp,
                        _ => {
                            return Err(ProcessError::NoSuchProvider(req.tmp_provider.clone()));
                        }
                    }
                };
                let info = provider.qr(&ctx.provider_context).await?;
                let rsp = QrAuthUrlRsp {
                    key: info.key,
                    url: info.url,
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetAlbumReq => {
            if let Some(Payload::GetAlbumReq(req)) = payload {
                let db = &ctx.provider_context.db;

                let (album, dy) = sqlm::album::Entity::find_by_id(req.id)
                    .find_also_related(sqlm::dynamic::Entity)
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchAlbum(req.id.to_string()))?;

                let artists = album.find_related(sqlm::artist::Entity).all(db).await?;

                let (songs, song_extras) = {
                    let songs = sqlm::song::Entity::find()
                        .filter(sqlm::song::Column::AlbumId.eq(album.id))
                        .order_by_asc(sqlm::song::Column::TrackNumber)
                        .all(db)
                        .await?;

                    to_rsp_songs(db, songs, Some(&album)).await?
                };

                let mut extra = prost_types::Struct::default();
                extra_insert_artists(&mut extra, &artists);
                if let Some(dy) = dy {
                    extra_insert_dynamic(&mut extra, &dy);
                }

                let rsp = msg::GetAlbumRsp {
                    item: Some(album.qcm_into()),
                    extra: Some(extra),
                    songs,
                    song_extras,
                };
                return Ok(rsp.qcm_into());
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

                let paginator = query.paginate(&ctx.provider_context.db, page_params.page_size);

                let total = paginator.num_items().await?;
                let albums = paginator.fetch_page(page_params.page).await?;

                let (items, extras) = to_rsp_albums(&ctx.provider_context.db, albums).await?;

                let rsp = GetAlbumsRsp {
                    items,
                    extras,
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
                    .paginate(&ctx.provider_context.db, page_params.page_size);

                let total = paginator.num_items().await?;
                let artists = paginator
                    .fetch_page(page_params.page)
                    .await?
                    .into_iter()
                    .map(|a| a.qcm_into())
                    .collect();

                let rsp = GetAlbumArtistsRsp {
                    items: artists,
                    extras: Vec::new(),
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
                    .paginate(&ctx.provider_context.db, page_params.page_size);

                let total = paginator.num_items().await?;
                let artists = paginator
                    .fetch_page(page_params.page)
                    .await?
                    .into_iter()
                    .map(|a| a.qcm_into())
                    .collect();

                let rsp = GetArtistsRsp {
                    items: artists,
                    extras: Vec::new(),
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetArtistReq => {
            if let Some(Payload::GetArtistReq(req)) = payload {
                let db = &ctx.provider_context.db;

                let artist = sqlm::artist::Entity::find_by_id(req.id)
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchArtist(req.id.to_string()))?;

                let rsp = msg::GetArtistRsp {
                    item: Some(artist.qcm_into()),
                    extra: None,
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetMixsReq => {
            if let Some(Payload::GetMixsReq(req)) = payload {
                let page_params = PageParams::new(req.page, req.page_size);

                let paginator = sqlm::mix::Entity::find()
                    .filter(sqlm::mix::Column::MixType.ne(MixType::Cache))
                    .qcm_filters(&req.filters, &req.filter_logics)
                    .paginate(&ctx.provider_context.db, page_params.page_size);

                let total = paginator.num_items().await?;
                let mixes = paginator
                    .fetch_page(page_params.page)
                    .await?
                    .into_iter()
                    .map(|m| m.qcm_into())
                    .collect();

                let rsp = msg::GetMixsRsp {
                    items: mixes,
                    extras: Vec::new(),
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
                    .paginate(&ctx.provider_context.db, page_params.page_size);

                let total = paginator.num_items().await?;
                let mixes = paginator
                    .fetch_page(page_params.page)
                    .await?
                    .into_iter()
                    .filter_map(|(_, m)| m.qcm_into())
                    .collect();

                let rsp = msg::GetRemoteMixsRsp {
                    items: mixes,
                    extras: Vec::new(),
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetMixReq => {
            if let Some(Payload::GetMixReq(req)) = payload {
                let db = &ctx.provider_context.db;

                let mix = sqlm::mix::Entity::find_by_id(req.id)
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchMix(req.id.to_string()))?;

                let rsp = msg::GetMixRsp {
                    item: Some(mix.qcm_into()),
                    extra: None,
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetMixSongsReq => {
            if let Some(Payload::GetMixSongsReq(req)) = payload {
                let db = &ctx.provider_context.db;

                if let Some((_, item)) = qcm_core::db::sync::check_cache_mix(db, req.id).await? {
                    let provider = global::provider(item.provider_id)
                        .ok_or(ProcessError::NoSuchProvider(item.provider_id.to_string()))?;
                    provider.sync_item(&ctx.provider_context, item).await?;
                }

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

                let paginator = query.paginate(db, page_params.page_size);

                let total = paginator.num_items().await?;
                let songs = paginator.fetch_page(page_params.page).await?;

                let (items, extras) = to_rsp_songs(db, songs, None).await?;

                let mix = sqlm::mix::Entity::find_by_id(req.id)
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchMix(req.id.to_string()))?;

                let rsp = msg::GetMixSongsRsp {
                    mix: Some(mix.qcm_into()),
                    items,
                    extras,
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::CreateMixReq => {
            if let Some(Payload::CreateMixReq(req)) = payload {
                let db = &ctx.provider_context.db;

                let new_mix = sqlm::mix::ActiveModel {
                    name: sea_orm::Set(req.name.clone()),
                    track_count: sea_orm::Set(0),
                    description: sea_orm::Set(String::new()),
                    mix_type: sea_orm::Set(MixType::Normal),
                    ..Default::default()
                };

                let res = sqlm::mix::Entity::insert(new_mix).exec(db).await?;

                let rsp = msg::CreateMixRsp {
                    id: res.last_insert_id,
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::DeleteMixReq => {
            if let Some(Payload::DeleteMixReq(req)) = payload {
                let db = &ctx.provider_context.db;
                sqlm::mix::Entity::delete_many()
                    .filter(sqlm::mix::Column::Id.is_in(req.ids.clone()))
                    .filter(sqlm::mix::Column::MixType.eq(MixType::Normal))
                    .exec(db)
                    .await?;

                sqlm::mix::Entity::update_many()
                    .col_expr(sqlm::mix::Column::MixType, Expr::val(MixType::Cache).into())
                    .filter(sqlm::mix::Column::Id.is_in(req.ids.clone()))
                    .filter(sqlm::mix::Column::MixType.eq(MixType::Link))
                    .exec(db)
                    .await?;
                return Ok(Rsp::default().qcm_into());
            }
        }
        MessageType::LinkMixReq => {
            if let Some(Payload::LinkMixReq(req)) = payload {
                let db = &ctx.provider_context.db;
                sqlm::mix::Entity::update_many()
                    .col_expr(sqlm::mix::Column::MixType, Expr::val(MixType::Link).into())
                    .filter(sqlm::mix::Column::Id.is_in(req.ids.clone()))
                    .exec(db)
                    .await?;
                return Ok(Rsp::default().qcm_into());
            }
        }
        MessageType::MixManipulateReq => {
            if let Some(Payload::MixManipulateReq(req)) = payload {
                let db = ctx.provider_context.db.begin().await?;

                let mut rsp = msg::MixManipulateRsp::default();
                match req.oper() {
                    msg::model::MixManipulateOper::AddSongs => {
                        let count = sqlm::mix::append_songs(&db, req.id, &req.song_ids).await?;

                        db.commit().await?;
                        rsp.count = count as i64;
                    }
                    msg::model::MixManipulateOper::RemoveSongs => {
                        let res = sqlm::rel_mix_song::Entity::delete_many()
                            .filter(sqlm::rel_mix_song::Column::SongId.is_in(req.song_ids.clone()))
                            .filter(sqlm::rel_mix_song::Column::MixId.eq(req.id))
                            .exec(&db)
                            .await?;

                        let count = res.rows_affected;
                        sqlm::mix::Entity::update_many()
                            .col_expr(
                                sqlm::mix::Column::TrackCount,
                                Expr::col(sqlm::mix::Column::TrackCount).sub(count).into(),
                            )
                            .exec(&db)
                            .await?;

                        db.commit().await?;
                        rsp.count = count as i64;
                    }
                    msg::model::MixManipulateOper::AddAlbums => {
                        let ids: Vec<i64> = sqlm::song::Entity::find()
                            .select_only()
                            .column(sqlm::song::Column::Id)
                            .filter(sqlm::song::Column::AlbumId.is_in(req.album_ids.clone()))
                            .into_tuple()
                            .all(&db)
                            .await?;

                        let count = sqlm::mix::append_songs(&db, req.id, &ids).await?;

                        db.commit().await?;
                        rsp.count = count as i64;
                    }
                    _ => {
                        return Err(ProcessError::NotImplemented);
                    }
                }
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetSongIdsReq => {
            if let Some(Payload::GetSongIdsReq(req)) = payload {
                let db = &ctx.provider_context.db;
                let mut ids = Vec::new();

                if !req.filters.is_empty() {
                    for f in &req.filters {
                        match f.payload {
                            Some(msg::filter::song_filter::Payload::MixIdFilter(id)) => {
                                if let Some((_, item)) =
                                    qcm_core::db::sync::check_cache_mix(db, id.value).await?
                                {
                                    let provider = global::provider(item.provider_id).ok_or(
                                        ProcessError::NoSuchProvider(item.provider_id.to_string()),
                                    )?;
                                    provider.sync_item(&ctx.provider_context, item).await?;
                                }
                            }
                            _ => {}
                        }
                    }

                    let query = sqlm::song::Entity::find()
                        .inner_join(sqlm::item::Entity)
                        .qcm_filters(&req.filters, &req.filter_logics);
                    ids = query
                        .select_only()
                        .column(sqlm::song::Column::Id)
                        .into_tuple()
                        .all(db)
                        .await?;
                } else {
                    use sea_query::{Alias, CommonTableExpression, Condition, Query, WithClause};
                    let song_sort: msg::model::SongSort = req
                        .sort
                        .try_into()
                        .map_err(|_| ProcessError::NotImplemented)?;

                    let album_sort: msg::model::AlbumSort = req
                        .album_sort
                        .try_into()
                        .map_err(|_| ProcessError::NotImplemented)?;

                    let cond: Condition = crate::db::filter::album_filters_to_condition(
                        &req.album_filters,
                        &req.album_filter_logics,
                    )
                    .unwrap_or_else(Condition::all);
                    let cte_query = Query::select()
                        .column((sqlm::album::Entity, sqlm::album::Column::Id))
                        .inner_join(sqlm::dynamic::Entity, sqlm::dynamic::Relation::Album.def())
                        .inner_join(sqlm::item::Entity, sqlm::item::Relation::Album.def())
                        .expr(Expr::cust_with_expr(
                            "row_number() OVER (ORDER BY ?)",
                            album_sort_col(album_sort),
                        ))
                        .from(sqlm::album::Entity)
                        .cond_where(cond)
                        .cond_where(
                            Condition::all()
                                .add(sqlm::dynamic::Column::IsExternal.eq(false))
                                .add(sqlm::item::Column::LibraryId.is_in(req.library_ids.clone())),
                        )
                        .to_owned();

                    let sorted_album_ids_alias = Alias::new("sorted_album_ids");
                    let sorted_album_id_alias = Alias::new("sorted_album_id");
                    let sorted_album_ord_alias = Alias::new("sorted_album_ord");
                    let with_clause = WithClause::new()
                        .cte(
                            CommonTableExpression::new()
                                .query(cte_query)
                                .table_name(sorted_album_ids_alias.clone())
                                .column(sorted_album_id_alias.clone())
                                .column(sorted_album_ord_alias.clone())
                                .to_owned(),
                        )
                        .to_owned();

                    let stmt = Query::select()
                        .column(sqlm::song::Column::Id)
                        .from(sqlm::song::Entity)
                        .inner_join(
                            sorted_album_ids_alias.clone(),
                            Expr::col((sqlm::song::Entity, sqlm::song::Column::AlbumId))
                                .equals(sorted_album_id_alias.clone()),
                        )
                        .order_by(sorted_album_ord_alias, req.album_asc.qcm_into())
                        .order_by_expr(song_sort_col(song_sort).into(), req.asc.qcm_into())
                        .to_owned()
                        .with(with_clause)
                        .to_owned();

                    let db_backend = ctx.provider_context.db.get_database_backend();
                    ids = db
                        .query_all(db_backend.build(&stmt))
                        .await?
                        .into_iter()
                        .filter_map(|col| col.try_get::<i64>("", "id").ok())
                        .collect();
                }
                let rsp = msg::GetSongIdsRsp { ids };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetSongsByIdReq => {
            if let Some(Payload::GetSongsByIdReq(req)) = payload {
                let db = &ctx.provider_context.db;
                let songs = sqlm::song::Entity::find()
                    .filter(sqlm::song::Column::Id.is_in(req.ids.clone()))
                    .all(db)
                    .await?;

                let (items, extras) = to_rsp_songs(db, songs, None).await?;

                let len = items.len();

                let rsp = msg::GetSongsByIdRsp {
                    items,
                    extras,
                    total: len as i32,
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetArtistAlbumReq => {
            if let Some(Payload::GetArtistAlbumReq(req)) = payload {
                let db = &ctx.provider_context.db;
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
                let paginator = albums_query.paginate(db, page_params.page_size);

                let total = paginator.num_items().await?;
                let albums = paginator.fetch_page(page_params.page).await?;

                let (items, extras) = to_rsp_albums(db, albums).await?;

                let rsp = msg::GetArtistAlbumRsp {
                    items,
                    extras,
                    total: total as i32,
                    has_more: page_params.has_more(total),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetSubtitleReq => {
            if let Some(Payload::GetSubtitleReq(req)) = payload {
                let db = &ctx.provider_context.db;

                let (native_id, provider_id): (String, i64) =
                    sqlm::song::Entity::find_by_id(req.song_id)
                        .inner_join(sqlm::item::Entity)
                        .select_only()
                        .column(sqlm::item::Column::NativeId)
                        .column(sqlm::item::Column::ProviderId)
                        .into_tuple()
                        .one(db)
                        .await?
                        .ok_or(ProcessError::NoSuchSong(req.song_id.to_string()))?;

                let provider = global::provider(provider_id)
                    .ok_or(ProcessError::NoSuchProvider(provider_id.to_string()))?;

                let subtitle = provider.subtitle(&native_id).await?;
                let rsp = msg::GetSubtitleRsp {
                    subtitle: Some(subtitle.qcm_into()),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::SearchReq => {
            if let Some(Payload::SearchReq(req)) = payload {
                let db = &ctx.provider_context.db;
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
                    let db_backend = ctx.provider_context.db.get_database_backend();
                    let item_table_et = sqlm::item::Entity::default();
                    let item_table = item_table_et.table_name();
                    Statement::from_sql_and_values(
                        db_backend,
                        format!(
                            r#"
                                    SELECT {table}.* FROM {table}
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
                            let albums_query =
                                sqlm::album::Entity::find().from_raw_sql(format_query(table, &fts));

                            let paginator = albums_query.paginate(db, page_params.page_size);
                            let total = paginator.num_items().await?;
                            let albums = paginator.fetch_page(page_params.page).await?;

                            let (items, extras) = to_rsp_albums(db, albums).await?;

                            albums_rsp = Some(GetAlbumsRsp {
                                items,
                                extras,
                                total: total as i32,
                                has_more: page_params.has_more(total),
                            });
                        }
                        msg::SearchType::Song => {
                            let entity = sqlm::song::Entity::default();
                            let table = entity.table_name();
                            let fts = format!("{table}_fts");
                            let query =
                                sqlm::song::Entity::find().from_raw_sql(format_query(table, &fts));

                            let paginator = query.paginate(db, page_params.page_size);
                            let total = paginator.num_items().await?;
                            let songs = paginator.fetch_page(page_params.page).await?;

                            let (items, extras) = to_rsp_songs(db, songs, None).await?;

                            songs_rsp = Some(GetSongsRsp {
                                items,
                                extras,
                                total: total as i32,
                                has_more: page_params.has_more(total),
                            });
                        }
                        msg::SearchType::Artist => {
                            let entity = sqlm::artist::Entity::default();
                            let table = entity.table_name();
                            let fts = format!("{table}_fts");
                            let query = sqlm::artist::Entity::find()
                                .from_raw_sql(format_query(table, &fts));
                            let page_params = PageParams::new(req.page, req.page_size);

                            let paginator = query.paginate(db, page_params.page_size);
                            let total = paginator.num_items().await?;

                            let artists = paginator
                                .fetch_page(page_params.page)
                                .await?
                                .into_iter()
                                .map(|a| a.qcm_into())
                                .collect();

                            artists_rsp = Some(GetArtistsRsp {
                                items: artists,
                                extras: Vec::new(),
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
        MessageType::SetFavoriteReq => {
            if let Some(Payload::SetFavoriteReq(req)) = payload {
                let db = &ctx.provider_context.db;
                use sqlm::type_enum::ItemType;
                let item_type: ItemType = req
                    .item_type
                    .try_into()
                    .map_err(|_| ProcessError::NoSuchItemType(req.item_type.to_string()))?;

                let (provider_id, native_id): (i64, String) = {
                    sqlm::item::Entity::find_by_id(req.id)
                        .select_only()
                        .column(sqlm::item::Column::ProviderId)
                        .column(sqlm::item::Column::NativeId)
                        .into_tuple()
                        .one(db)
                        .await?
                        .ok_or(ProcessError::NotFound)?
                };

                let provider = global::provider(provider_id)
                    .ok_or(ProcessError::NoSuchProvider(provider_id.to_string()))?;

                provider
                    .favorite(&ctx.provider_context, &native_id, item_type, req.value)
                    .await?;

                use sea_orm::Set;

                match item_type {
                    ItemType::Album | ItemType::Song | ItemType::Artist => {
                        let m = sqlm::dynamic::ActiveModel {
                            id: Set(req.id),
                            favorite_at: Set(match req.value {
                                true => Some(Timestamp::now()),
                                false => None,
                            }),
                            ..Default::default()
                        };

                        sqlm::dynamic::Entity::insert(m)
                            .on_conflict(
                                sea_query::OnConflict::columns([sqlm::dynamic::Column::Id])
                                    .update_column(sqlm::dynamic::Column::FavoriteAt)
                                    .to_owned(),
                            )
                            .exec(db)
                            .await?;
                    }
                    _ => {}
                }
                return Ok(Rsp::default().qcm_into());
            }
        }
        MessageType::GetStorageInfoReq => {
            if let Some(Payload::GetStorageInfoReq(_)) = payload {
                let db = &ctx.provider_context.cache_db;

                let size: Vec<(i32, i64)> = sqlm::cache::Entity::find()
                    .select_only()
                    .column(sqlm::cache::Column::CacheType)
                    .column_as(Expr::col(sqlm::cache::Column::ContentLength).sum(), "size")
                    .group_by(sqlm::cache::Column::CacheType)
                    .into_tuple()
                    .all(db)
                    .await?;

                let db_size = qcm_core::db::size::sqlite_sizes(&ctx.provider_context.db).await?;
                let cache_db_size =
                    qcm_core::db::size::sqlite_sizes(&ctx.provider_context.cache_db).await?;

                let mut rsp = GetStorageInfoRsp {
                    media_size: 0,
                    image_size: 0,
                    database_size: db_size
                        .total_disk_bytes
                        .saturating_add(cache_db_size.total_disk_bytes)
                        as i64,
                };

                for (content_type, size) in size {
                    match content_type {
                        x if x == sqlm::type_enum::CacheType::Image as i32 => rsp.image_size = size,
                        x if x == sqlm::type_enum::CacheType::Audio as i32 => rsp.media_size = size,
                        _ => {}
                    }
                }

                return Ok(rsp.qcm_into());
            }
        }
        MessageType::PlaylogReq => {
            if let Some(Payload::PlaylogReq(req)) = payload {
                use sea_orm::Set;

                let action = msg::model::PlaylogAction::try_from(req.action)
                    .unwrap_or(msg::model::PlaylogAction::Unspecified);

                let item_info: Option<(i64, String)> =
                    sqlm::item::Entity::find_by_id(req.song_id)
                        .select_only()
                        .column(sqlm::item::Column::ProviderId)
                        .column(sqlm::item::Column::NativeId)
                        .into_tuple()
                        .one(&ctx.provider_context.db)
                        .await?;

                if let Some((provider_id, native_id)) = &item_info {
                    match action {
                        msg::model::PlaylogAction::Play => {
                            global::set_playing(
                                *provider_id,
                                global::PlayingInfo {
                                    song_id: req.song_id,
                                    native_id: native_id.clone(),
                                },
                            );
                        }
                        msg::model::PlaylogAction::Stop => {
                            global::clear_playing(*provider_id);
                        }
                        _ => {}
                    }
                }

                let album_id: Option<i64> = sqlm::song::Entity::find_by_id(req.song_id)
                    .select_only()
                    .column(sqlm::song::Column::AlbumId)
                    .into_tuple()
                    .one(&ctx.provider_context.db)
                    .await?;

                let mut dys = Vec::new();
                dys.push(sqlm::dynamic::ActiveModel {
                    id: Set(req.song_id),
                    last_played_at: Set(Some(Timestamp::from_millis(req.timestamp))),
                    play_count: Set(1),
                    ..Default::default()
                });
                if let Some(album_id) = album_id {
                    dys.push(sqlm::dynamic::ActiveModel {
                        id: Set(album_id),
                        last_played_at: Set(Some(Timestamp::from_millis(req.timestamp))),
                        play_count: Set(1),
                        ..Default::default()
                    });
                }

                sqlm::dynamic::Entity::insert_many(dys)
                    .on_conflict(
                        sea_query::OnConflict::columns([sqlm::dynamic::Column::Id])
                            .update_column(sqlm::dynamic::Column::LastPlayedAt)
                            .value(
                                sqlm::dynamic::Column::PlayCount,
                                Expr::col(sqlm::dynamic::Column::PlayCount).add(1),
                            )
                            .to_owned(),
                    )
                    .exec(&ctx.provider_context.db)
                    .await?;
            }
        }
        MessageType::SyncReq => {
            if let Some(Payload::SyncReq(req)) = payload {
                let (tx, rx) = oneshot::channel::<i64>();
                ctx.provider_context
                    .ev_sender
                    .send(CoreEvent::ProviderSync {
                        id: req.provider_id,
                        oneshot: Some(tx),
                    })
                    .await?;
                let task_id = rx.await;
                let rsp = SyncRsp {
                    handle: task_id.unwrap_or(-1),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::SyncItemReq => {
            if let Some(Payload::SyncItemReq(req)) = payload {
                let db = &ctx.provider_context.db;

                let item = sqlm::item::Entity::find_by_id(req.id)
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NotFound)?;

                let provider_id = item.provider_id;

                let provider = global::provider(provider_id)
                    .ok_or(ProcessError::NoSuchProvider(provider_id.to_string()))?;

                provider.sync_item(&ctx.provider_context, item).await?;

                return Ok(Rsp::default().qcm_into());
            }
        }
        MessageType::TestReq => {
            if let Some(Payload::TestReq(req)) = payload {
                let rsp = TestRsp {
                    test_data: format!("Echo: {}", req.test_data),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetQueueNextReq => {
            if let Some(Payload::GetQueueNextReq(req)) = payload {
                let (queue_native_id, queue_provider_id): (String, i64) =
                    sqlm::item::Entity::find_by_id(req.queue_id)
                        .select_only()
                        .column(sqlm::item::Column::NativeId)
                        .column(sqlm::item::Column::ProviderId)
                        .into_tuple()
                        .one(&ctx.provider_context.db)
                        .await?
                        .ok_or(ProcessError::NoSuchProvider(req.queue_id.to_string()))?;

                let current_native_ids: Vec<String> = sqlm::item::Entity::find()
                    .select_only()
                    .column(sqlm::item::Column::NativeId)
                    .filter(sqlm::item::Column::Id.is_in(req.current_song_ids.clone()))
                    .into_tuple()
                    .all(&ctx.provider_context.db)
                    .await?;

                let provider = global::provider(queue_provider_id)
                    .ok_or(ProcessError::NoSuchProvider(queue_provider_id.to_string()))?;
                let new_songs = provider
                    .get_queue_next(
                        &ctx.provider_context,
                        &queue_native_id,
                        &current_native_ids,
                    )
                    .await?;
                drop(provider);

                // provider.get_queue_next 内部已 sync songs 并返回带 DB id 的 models，
                // 直接按 id 读回即可，不要再 sync_songs（会把 DB id 当 native_id 写坏数据）
                let ids: Vec<i64> = new_songs.iter().map(|s| s.id).collect();
                let db_songs = sqlm::song::Entity::find()
                    .filter(sqlm::song::Column::Id.is_in(ids))
                    .all(&ctx.provider_context.db)
                    .await?;

                let (songs, extras) =
                    to_rsp_songs(&ctx.provider_context.db, db_songs, None).await?;
                let rsp = msg::GetQueueNextRsp { songs, extras };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetRadioQueuesReq => {
            if let Some(Payload::GetRadioQueuesReq(req)) = payload {
                let queues = sqlm::remote_mix::Entity::find()
                    .inner_join(sqlm::item::Entity)
                    .filter(
                        Expr::col((sqlm::item::Entity, sqlm::item::Column::ProviderId))
                            .is_in(req.provider_ids.clone()),
                    )
                    .filter(
                        Expr::col((sqlm::item::Entity, sqlm::item::Column::Type))
                            .eq(sqlm::type_enum::ItemType::Radio as i32),
                    )
                    .filter(sqlm::remote_mix::Column::MixType.eq("queue"))
                    .all(&ctx.provider_context.db)
                    .await?;

                log::info!("provider id: {:?}, queus count: {}", req.provider_ids, queues.len());

                let queues = queues
                    .into_iter()
                    .map(|q| msg::model::RadioQueue {
                        id: q.id,
                        name: q.name,
                        description: q.description.unwrap_or_default(),
                    })
                    .collect();

                let rsp = msg::GetRadioQueuesRsp { queues };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetHomeBlocksReq => {
            if let Some(Payload::GetHomeBlocksReq(req)) = payload {
                let provider = global::provider(req.provider_id)
                    .ok_or(ProcessError::NoSuchProvider(req.provider_id.to_string()))?;
                let blocks = provider.home_blocks(&ctx.provider_context).await?;
                let rsp = msg::GetHomeBlocksRsp {
                    blocks: blocks.into_iter().map(|b| b.qcm_into()).collect(),
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetHomeBlockItemsReq => {
            if let Some(Payload::GetHomeBlockItemsReq(req)) = payload {
                let provider = global::provider(req.provider_id)
                    .ok_or(ProcessError::NoSuchProvider(req.provider_id.to_string()))?;
                let page_params = PageParams::new(req.page, req.page_size);
                let (content, total) = provider
                    .home_block_items(
                        &ctx.provider_context,
                        &req.block_id,
                        page_params.page as i32,
                        page_params.page_size as i32,
                    )
                    .await?;
                let total_u = total as u64;
                let has_more = page_params.has_more(total_u);
                use qcm_core::provider::HomeBlockContent as C;
                use msg::get_home_block_items_rsp::Content;
                let content = match content {
                    C::Albums(items) => Content::Albums(msg::GetAlbumsRsp {
                        items: items.into_iter().map(|i| i.qcm_into()).collect(),
                        extras: vec![],
                        total,
                        has_more,
                    }),
                    C::Mixes(items) => Content::Mixes(msg::GetMixsRsp {
                        items: items.into_iter().map(|i| i.qcm_into()).collect(),
                        extras: vec![],
                        total,
                        has_more,
                    }),
                    C::Artists(items) => Content::Artists(msg::GetArtistsRsp {
                        items: items.into_iter().map(|i| i.qcm_into()).collect(),
                        extras: vec![],
                        total,
                        has_more,
                    }),
                    C::Songs(items) => Content::Songs(msg::GetSongsRsp {
                        items: items.into_iter().map(|i| i.qcm_into()).collect(),
                        extras: vec![],
                        total,
                        has_more,
                    }),
                };
                let rsp = msg::GetHomeBlockItemsRsp {
                    content: Some(content),
                };
                return Ok(rsp.qcm_into());
            }
        }
        _ => {
            return Err(ProcessError::UnsupportedMessageType(mtype.into()));
        }
    }
    return Err(ProcessError::UnexpectedPayload(mtype.into()));
}
