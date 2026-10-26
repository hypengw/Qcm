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
    prelude::Expr, sea_query, ColumnTrait, ConnectionTrait, EntityTrait, QueryFilter, QuerySelect,
    RelationTrait,
};

use crate::api::{
    helper_sort::{album_sort_col, order_songs, song_sort_col},
    pagination::PageParams,
};
use crate::convert::QcmInto;
use crate::db::filter::SelectQcmMsgFilters;
use crate::error::ProcessError;
use crate::event::{BackendEvent, ServiceContext};
use crate::msg::{
    self, AuthProviderRsp, GetProviderMetasRsp, GetStorageInfoRsp, MessageType, QcmMessage,
    QrAuthUrlRsp, Rsp, SyncRsp, TestRsp,
};

pub async fn process_qcm(
    ctx: &Arc<ServiceContext>,
    data: &[u8],
    in_id: &mut Option<i32>,
) -> Result<QcmMessage, ProcessError> {
    let message = QcmMessage::decode(data)?;
    *in_id = Some(message.id);

    if let Some(msg::qcm_message::Payload::GetMixSongsReq(req)) = &message.payload {
        ctx.mix_refresh
            .refresh(&ctx.provider_context, req.id, req.force_refresh)
            .await?;
    }
    if super::entity_query::handles(message.r#type()) {
        return super::entity::query(ctx, &message).await;
    }
    let mut response = process_request(ctx, &message).await?;
    super::entity::populate(ctx, &message, &mut response).await?;
    Ok(response)
}

async fn process_request(
    ctx: &Arc<ServiceContext>,
    message: &QcmMessage,
) -> Result<QcmMessage, ProcessError> {
    use msg::qcm_message::Payload;
    let mtype = message.r#type();
    let payload = &message.payload;

    match mtype {
        MessageType::GetNetworkSettingsReq => {
            return Ok(crate::network::read(&ctx.provider_context.db)
                .await?
                .qcm_into());
        }
        MessageType::UpdateNetworkSettingsReq => {
            if let Some(Payload::UpdateNetworkSettingsReq(req)) = payload {
                return Ok(crate::network::update(&ctx.provider_context.db, req)
                    .await?
                    .qcm_into());
            }
        }
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
                let write = ctx
                    .provider_context
                    .sync_coordinator
                    .write(id, None)
                    .await?;
                write.invalidate_sync();
                sqlm::provider::Entity::delete_by_id(id)
                    .exec(&ctx.provider_context.db)
                    .await?;
                write.invalidate_scope()?;
                drop(write);
                ctx.playback.invalidate(id).await;
                global::remove_provider(id);
                crate::network::restore(&ctx.provider_context.db).await?;
                ctx.radio.invalidate(&ctx.provider_context, id).await;
                ctx.resource_cache.invalidate(id).await?;
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
                let provider = global::provider(req.provider_id)
                    .ok_or(ProcessError::NoSuchProvider(req.provider_id.to_string()))?;
                let settings = provider.settings();
                let update = msg::UpdateProviderReq {
                    provider_id: req.provider_id,
                    name: provider.name(),
                    settings_json: serde_json::to_string(&settings.values)
                        .map_err(anyhow::Error::from)?,
                    settings_revision: settings.revision,
                    tmp_provider: req.tmp_provider.clone(),
                    ..Default::default()
                };
                let result = crate::provider_settings::update(ctx, &update).await?;
                let rsp = if result.code == msg::model::AuthResult::Ok as i32 {
                    Rsp::default()
                } else {
                    Rsp {
                        code: msg::ErrorCode::Internal as i32,
                        message: result.message,
                        ..Default::default()
                    }
                };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::UpdateProviderReq => {
            if let Some(Payload::UpdateProviderReq(req)) = payload {
                return Ok(crate::provider_settings::update(ctx, req).await?.qcm_into());
            }
        }
        MessageType::GetProviderSettingsReq => {
            if let Some(Payload::GetProviderSettingsReq(req)) = payload {
                return Ok(crate::provider_settings::read(req.provider_id)?.qcm_into());
            }
        }
        MessageType::CreateTmpProviderReq => {
            if let Some(Payload::CreateTmpProviderReq(req)) = payload {
                let meta = global::provider_meta(&req.type_name)
                    .ok_or(ProcessError::NoSuchProviderType(req.type_name.clone()))?;
                let provider = (meta.creator)(None, "", &global::device_id())?;
                if let Some(id) = req.network_provider_id {
                    let source =
                        global::provider(id).ok_or(ProcessError::NoSuchProvider(id.to_string()))?;
                    if source.type_name() != req.type_name {
                        return Err(ProcessError::NotAuth);
                    }
                    provider.set_network_scope(Some(id));
                }
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

        MessageType::CreateMixReq => {
            if let Some(Payload::CreateMixReq(req)) = payload {
                let db = &ctx.provider_context.db;

                let _write = ctx.provider_context.sync_coordinator.write(0, None).await?;
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
                let _writes = ctx
                    .provider_context
                    .sync_coordinator
                    .write_mixes(db, &req.ids, &[])
                    .await?;
                sqlm::mix::Entity::delete_many()
                    .filter(sqlm::mix::Column::Id.is_in(req.ids.clone()))
                    .filter(sqlm::mix::Column::MixType.eq(MixType::Normal))
                    .exec(db)
                    .await?;

                sqlm::mix::Entity::update_many()
                    .col_expr(sqlm::mix::Column::MixType, Expr::val(MixType::Cache).into())
                    .col_expr(
                        sqlm::mix::Column::UpdateAt,
                        Expr::val(Timestamp::now()).into(),
                    )
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
                let _writes = ctx
                    .provider_context
                    .sync_coordinator
                    .write_mixes(db, &req.ids, &[])
                    .await?;
                sqlm::mix::Entity::update_many()
                    .col_expr(sqlm::mix::Column::MixType, Expr::val(MixType::Link).into())
                    .col_expr(
                        sqlm::mix::Column::UpdateAt,
                        Expr::val(Timestamp::now()).into(),
                    )
                    .filter(sqlm::mix::Column::Id.is_in(req.ids.clone()))
                    .exec(db)
                    .await?;
                return Ok(Rsp::default().qcm_into());
            }
        }
        MessageType::MixManipulateReq => {
            if let Some(Payload::MixManipulateReq(req)) = payload {
                let mut affected = req.song_ids.clone();
                affected.extend_from_slice(&req.album_ids);
                let _writes = ctx
                    .provider_context
                    .sync_coordinator
                    .write_mixes(&ctx.provider_context.db, &[req.id], &affected)
                    .await?;
                let db = ctx.provider_context.db.begin().await?;

                let mut rsp = msg::MixManipulateRsp::default();
                match req.oper() {
                    msg::model::MixManipulateOper::AddSongs => {
                        let count = sqlm::mix::append_songs(&db, req.id, &req.song_ids).await?;

                        db.commit().await?;
                        rsp.count = count as i64;
                    }
                    msg::model::MixManipulateOper::RemoveSongs => {
                        let count = sqlm::mix::remove_songs(&db, req.id, &req.song_ids).await?;

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
                let song_sort: msg::model::SongSort = req
                    .sort
                    .try_into()
                    .map_err(|_| ProcessError::NotImplemented)?;

                if !req.filters.is_empty() {
                    for f in &req.filters {
                        match f.payload {
                            Some(msg::filter::song_filter::Payload::MixIdFilter(id)) => {
                                ctx.mix_refresh
                                    .refresh(&ctx.provider_context, id.value, false)
                                    .await?;
                            }
                            _ => {}
                        }
                    }

                    let query = order_songs(
                        sqlm::song::Entity::find()
                            .inner_join(sqlm::item::Entity)
                            .qcm_filters(&req.filters, &req.filter_logics),
                        song_sort,
                        req.asc.qcm_into(),
                    );
                    ids = query
                        .select_only()
                        .column(sqlm::song::Column::Id)
                        .into_tuple()
                        .all(db)
                        .await?;
                } else {
                    use sea_query::{Alias, CommonTableExpression, Condition, Query, WithClause};
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

        MessageType::SetFavoriteReq => {
            if let Some(Payload::SetFavoriteReq(req)) = payload {
                let db = &ctx.provider_context.db;
                use sqlm::type_enum::ItemType;
                let mut item_type: ItemType = req
                    .item_type
                    .try_into()
                    .map_err(|_| ProcessError::NoSuchItemType(req.item_type.to_string()))?;

                if item_type == ItemType::AlbumArtist {
                    item_type = ItemType::Artist;
                }
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
                let mut write = ctx
                    .provider_context
                    .sync_coordinator
                    .write(provider_id, None)
                    .await?;
                write.favorite(req.id);
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
                let size = ctx.resource_cache.usage().await?;

                let db_size = qcm_core::db::size::sqlite_sizes(&ctx.provider_context.db).await?;
                let cache_db_size =
                    qcm_core::db::size::sqlite_sizes(&ctx.provider_context.cache_db).await?;

                let rsp = GetStorageInfoRsp {
                    media_size: size.audio as i64,
                    image_size: size.image as i64,
                    database_size: db_size
                        .total_disk_bytes
                        .saturating_add(cache_db_size.total_disk_bytes)
                        .saturating_add(size.metadata) as i64,
                };

                return Ok(rsp.qcm_into());
            }
        }
        MessageType::PlaylogReq => {
            if let Some(Payload::PlaylogReq(req)) = payload {
                use sea_orm::Set;
                let _writes = ctx
                    .provider_context
                    .sync_coordinator
                    .write_items(&ctx.provider_context.db, &[req.song_id])
                    .await?;

                let action = msg::model::PlaylogAction::try_from(req.action)
                    .unwrap_or(msg::model::PlaylogAction::Unspecified);

                let item_info: Option<(i64, String)> = sqlm::item::Entity::find_by_id(req.song_id)
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
        MessageType::OpenQueueReq => {
            if let Some(Payload::OpenQueueReq(req)) = payload {
                return Ok(super::radio::open(ctx, req).await?.qcm_into());
            }
        }
        MessageType::CloseQueueReq => {
            if let Some(Payload::CloseQueueReq(req)) = payload {
                ctx.radio
                    .close(&ctx.provider_context, &req.session_id)
                    .await;
                return Ok(Rsp::default().qcm_into());
            }
        }
        MessageType::GetQueueNextReq => {
            if let Some(Payload::GetQueueNextReq(req)) = payload {
                return Ok(super::radio::next(ctx, req).await?.qcm_into());
            }
        }
        MessageType::GetRadioQueuesReq => {
            if let Some(Payload::GetRadioQueuesReq(req)) = payload {
                let result =
                    crate::home::radio_queues(&ctx.provider_context, &req.provider_ids).await?;
                let rsp = msg::GetRadioQueuesRsp { queues: result };
                return Ok(rsp.qcm_into());
            }
        }
        MessageType::GetHomeRecommendationsReq => {
            if let Some(Payload::GetHomeRecommendationsReq(req)) = payload {
                return Ok(
                    crate::home::recommendations(&ctx.provider_context, &req.provider_ids)
                        .await
                        .qcm_into(),
                );
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
                use msg::get_home_block_items_rsp::Content;
                use qcm_core::provider::HomeBlockContent as C;
                let content = match content {
                    C::Albums(items) => Content::Albums(msg::GetAlbumsRsp {
                        ids: items.into_iter().map(|i| i.id).collect(),
                        total,
                        has_more,
                    }),
                    C::Mixes(items) => Content::Mixes(msg::GetMixsRsp {
                        ids: items.into_iter().map(|i| i.id).collect(),
                        total,
                        has_more,
                    }),
                    C::Artists(items) => Content::Artists(msg::GetArtistsRsp {
                        ids: items.into_iter().map(|i| i.id).collect(),
                        total,
                        has_more,
                    }),
                    C::Songs(items) => Content::Songs(msg::GetSongsRsp {
                        ids: items.into_iter().map(|i| i.id).collect(),
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
