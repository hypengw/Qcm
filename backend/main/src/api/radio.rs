use super::helper_extra::to_rsp_songs;
use crate::{
    error::ProcessError,
    event::ServiceContext,
    msg,
    radio::{Page, Target},
};
use qcm_core::{
    global,
    model::{self as sqlm, type_enum::ItemType},
    provider::{QueueInput, QueueOpenReason, QueueOpenRequest},
};
use sea_orm::{ColumnTrait, EntityTrait, QueryFilter};
use std::{collections::HashMap, sync::Arc};

async fn target(ctx: &ServiceContext, queue_id: i64) -> Result<Target, ProcessError> {
    let item = sqlm::item::Entity::find_by_id(queue_id)
        .one(&ctx.provider_context.db)
        .await?
        .ok_or(ProcessError::NotFound)?;
    if item.r#type != ItemType::Radio {
        return Err(ProcessError::UnsupportedItemType(item.r#type.to_string()));
    }
    let provider = global::provider(item.provider_id)
        .ok_or(ProcessError::NoSuchProvider(item.provider_id.to_string()))?;
    Ok(Target {
        queue_id,
        native_id: item.native_id,
        provider,
    })
}

pub async fn open(
    ctx: &Arc<ServiceContext>,
    req: &msg::OpenQueueReq,
) -> Result<msg::OpenQueueRsp, ProcessError> {
    let target = target(ctx, req.queue_id).await?;
    let input = if req.input_id != 0 {
        let item = sqlm::item::Entity::find_by_id(req.input_id)
            .one(&ctx.provider_context.db)
            .await?
            .ok_or(ProcessError::NotFound)?;
        if Some(item.provider_id) != target.provider.id() {
            return Err(ProcessError::WrongId(req.input_id.to_string()));
        }
        Some(QueueInput {
            native_id: item.native_id,
            item_type: item.r#type,
        })
    } else {
        None
    };
    let reason = match msg::QueueOpenReason::try_from(req.reason)
        .map_err(|_| ProcessError::MissingFields("radio reason".into()))?
    {
        msg::QueueOpenReason::Initial => QueueOpenReason::Initial,
        msg::QueueOpenReason::Reconfigure => QueueOpenReason::Reconfigure,
        msg::QueueOpenReason::Restart => QueueOpenReason::Restart,
    };
    let provider_id = target.provider.id().ok_or(ProcessError::NotFound)?;
    let page = ctx
        .radio
        .open(
            ctx.provider_context.clone(),
            target,
            req.operation_id.clone(),
            req.previous_session_id.clone(),
            QueueOpenRequest {
                configuration: req.configuration.clone(),
                input,
                reason,
                previous_state: None,
            },
        )
        .await?;
    let id = page.session_id.clone();
    let response = response(ctx, page, provider_id).await;
    if !response
        .as_ref()
        .is_ok_and(|(songs, _, _)| !songs.is_empty())
    {
        ctx.radio.close(&ctx.provider_context, &id).await;
        return Err(response.err().unwrap_or_else(|| {
            ProcessError::Internal(anyhow::anyhow!("Radio returned no playable songs"))
        }));
    }
    let (songs, extras, session) = response?;
    Ok(msg::OpenQueueRsp {
        songs,
        extras,
        session: Some(session),
    })
}

pub async fn next(
    ctx: &Arc<ServiceContext>,
    req: &msg::GetQueueNextReq,
) -> Result<msg::GetQueueNextRsp, ProcessError> {
    let target = target(ctx, req.queue_id)
        .await
        .or_else(|error| match error {
            ProcessError::NotFound | ProcessError::NoSuchProvider(_)
                if !req.session_id.is_empty() =>
            {
                Err(crate::radio::RadioError::Expired.into())
            }
            error => Err(error),
        })?;
    let items = sqlm::item::Entity::find()
        .filter(sqlm::item::Column::Id.is_in(req.current_song_ids.clone()))
        .filter(sqlm::item::Column::ProviderId.eq(target.provider.id()))
        .filter(sqlm::item::Column::Type.eq(ItemType::Song))
        .all(&ctx.provider_context.db)
        .await?;
    let items: HashMap<_, _> = items.into_iter().map(|i| (i.id, i.native_id)).collect();
    let current = req
        .current_song_ids
        .iter()
        .filter_map(|id| items.get(id).cloned())
        .collect::<Vec<_>>();
    let provider_id = target.provider.id().ok_or(ProcessError::NotFound)?;
    if req.session_id.is_empty() {
        let songs = target
            .provider
            .get_queue_next_configured(
                &ctx.provider_context,
                &target.native_id,
                &current,
                &req.configuration,
            )
            .await?;
        let (songs, extras) = ordered_songs(ctx, songs, provider_id).await?;
        return Ok(msg::GetQueueNextRsp {
            songs,
            extras,
            session: None,
        });
    }
    let page = ctx
        .radio
        .next(
            ctx.provider_context.clone(),
            target,
            req.session_id.clone(),
            req.operation_id.clone(),
            req.expected_version,
            current,
        )
        .await?;
    let (songs, extras, session) = response(ctx, page, provider_id).await?;
    Ok(msg::GetQueueNextRsp {
        songs,
        extras,
        session: Some(session),
    })
}

async fn ordered_songs(
    ctx: &ServiceContext,
    songs: Vec<sqlm::song::Model>,
    provider_id: i64,
) -> Result<(Vec<msg::model::Song>, Vec<prost_types::Struct>), ProcessError> {
    let ids: Vec<_> = songs.iter().map(|s| s.id).collect();
    let songs = sqlm::song::Entity::find()
        .inner_join(sqlm::item::Entity)
        .filter(sqlm::item::Column::ProviderId.eq(provider_id))
        .filter(sqlm::song::Column::Id.is_in(ids.clone()))
        .all(&ctx.provider_context.db)
        .await?;
    let mut songs: HashMap<_, _> = songs.into_iter().map(|s| (s.id, s)).collect();
    to_rsp_songs(
        &ctx.provider_context.db,
        ids.into_iter().filter_map(|id| songs.remove(&id)).collect(),
        None,
    )
    .await
}

async fn response(
    ctx: &ServiceContext,
    page: Page,
    provider_id: i64,
) -> Result<
    (
        Vec<msg::model::Song>,
        Vec<prost_types::Struct>,
        msg::model::QueueSession,
    ),
    ProcessError,
> {
    let (songs, extras) = ordered_songs(ctx, page.batch.songs, provider_id).await?;
    if let Some(source_id) = page.batch.presentation.source_item_id {
        let source = sqlm::item::Entity::find_by_id(source_id)
            .one(&ctx.provider_context.db)
            .await?;
        if source.is_none_or(|item| item.provider_id != provider_id) {
            return Err(ProcessError::WrongId(source_id.to_string()));
        }
    }
    let session = msg::model::QueueSession {
        id: page.session_id,
        version: page.version,
        configuration: page.batch.configuration,
        presentation: Some(msg::model::QueuePresentation {
            description: page.batch.presentation.description,
            source_item_id: page.batch.presentation.source_item_id.unwrap_or_default(),
        }),
        exhausted: page.batch.exhausted,
    };
    Ok((songs, extras, session))
}
