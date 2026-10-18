use crate::{
    convert::QcmInto,
    error::ProcessError,
    msg::{self, model as wire},
};
use qcm_core::{
    error::ProviderError,
    global,
    model::{self as m, type_enum::ItemType},
    provider::{CardAction, CardCover, CardStyle, Context, ForYouEntry, Provider},
};
use sea_orm::{
    prelude::Expr, ColumnTrait, ConnectionTrait, EntityTrait, QueryFilter, QueryOrder,
    TransactionTrait,
};
use std::{
    collections::{HashMap, HashSet},
    sync::Arc,
};
use wire::for_you_card::Target;

async fn radio_rows(
    db: &impl ConnectionTrait,
    ids: &[i64],
) -> Result<Vec<(m::remote_mix::Model, m::item::Model)>, ProcessError> {
    let rows = m::remote_mix::Entity::find()
        .inner_join(m::item::Entity)
        .filter(
            Expr::col((m::item::Entity, m::item::Column::ProviderId)).is_in(ids.iter().copied()),
        )
        .filter(Expr::col((m::item::Entity, m::item::Column::Type)).eq(ItemType::Radio as i32))
        .filter(m::remote_mix::Column::MixType.eq("queue"))
        .order_by_asc(m::remote_mix::Column::Id)
        .select_also(m::item::Entity)
        .all(db)
        .await?;
    rows.into_iter()
        .map(|(q, item)| Ok((q, item.ok_or(ProcessError::NotFound)?)))
        .collect()
}

pub async fn radio_queues(
    ctx: &Context,
    ids: &[i64],
) -> Result<Vec<wire::RadioQueue>, ProcessError> {
    let rows = radio_rows(&ctx.db, ids).await?;
    let mut result = Vec::with_capacity(rows.len());
    for (q, item) in rows {
        let provider = global::provider(item.provider_id)
            .ok_or_else(|| ProcessError::NoSuchProvider(item.provider_id.to_string()))?;
        let definition = provider.queue_definition(ctx, &item.native_id).await?;
        result.push(wire::RadioQueue {
            id: q.id,
            name: q.name,
            description: q.description.unwrap_or_default(),
            configuration: definition
                .configuration
                .into_iter()
                .map(QcmInto::qcm_into)
                .collect(),
            can_restart: definition.can_restart,
            restart_label: definition.restart_label,
        });
    }
    Ok(result)
}

fn invalid(message: impl Into<String>) -> ProcessError {
    ProcessError::Internal(qcm_core::anyhow!(message.into()))
}
fn action(value: CardAction) -> i32 {
    (match value {
        CardAction::OpenItem => wire::CardAction::OpenItem,
        CardAction::PlayItem => wire::CardAction::PlayItem,
        CardAction::ActivateRadio => wire::CardAction::ActivateRadio,
    }) as i32
}
fn identity(target: &Target) -> (ItemType, i64, &str) {
    match target {
        Target::Mix(v) => (ItemType::Mix, v.id, &v.name),
        Target::Album(v) => (ItemType::Album, v.id, &v.name),
        Target::Artist(v) => (ItemType::Artist, v.id, &v.name),
        Target::Song(v) => (ItemType::Song, v.id, &v.name),
        Target::Radio(v) => (ItemType::Radio, v.id, &v.name),
    }
}
async fn target(
    db: &sea_orm::DatabaseTransaction,
    item: &m::item::Model,
    radios: &HashMap<i64, wire::RadioQueue>,
) -> Result<Option<Target>, ProcessError> {
    Ok(match item.r#type {
        ItemType::Mix => m::mix::Entity::find()
            .filter(m::mix::Column::RemoteId.eq(item.id))
            .one(db)
            .await?
            .map(|v| Target::Mix(v.qcm_into())),
        ItemType::Album => m::album::Entity::find_by_id(item.id)
            .one(db)
            .await?
            .map(|v| Target::Album(v.qcm_into())),
        ItemType::Artist => m::artist::Entity::find_by_id(item.id)
            .one(db)
            .await?
            .map(|v| Target::Artist(v.qcm_into())),
        ItemType::Song => m::song::Entity::find_by_id(item.id)
            .one(db)
            .await?
            .map(|v| Target::Song(v.qcm_into())),
        ItemType::Radio => radios.get(&item.id).cloned().map(Target::Radio),
        _ => return Err(invalid("unsupported For You target type")),
    })
}

async fn fallback(ctx: &Context, provider: i64) -> Result<Vec<ForYouEntry>, ProcessError> {
    let rows = m::remote_mix::Entity::find()
        .inner_join(m::item::Entity)
        .filter(Expr::col((m::item::Entity, m::item::Column::ProviderId)).eq(provider))
        .filter(Expr::col((m::item::Entity, m::item::Column::Type)).eq(ItemType::Mix as i32))
        .filter(m::remote_mix::Column::MixType.eq("daily"))
        .order_by_asc(m::remote_mix::Column::Id)
        .all(&ctx.db)
        .await?;
    Ok(rows
        .into_iter()
        .map(|row| ForYouEntry {
            entry_key: format!("daily/{}", row.id),
            target: qcm_core::db::sync_batch::ItemRef::Id(row.id),
            presentation: Default::default(),
            primary_action: CardAction::OpenItem,
            play_action: Some(CardAction::PlayItem),
        })
        .collect())
}

async fn candidate(
    ctx: &Context,
    provider_id: i64,
    provider: Arc<dyn Provider>,
    epoch: u64,
) -> Result<wire::HomeRecommendation, ProcessError> {
    let revision = ctx.sync_coordinator.read_revision(provider_id).await;
    let entries = match provider.for_you_items(ctx).await {
        Ok(entries) => entries,
        Err(ProviderError::NotImplemented) => fallback(ctx, provider_id).await?,
        Err(e) => return Err(e.into()),
    };
    if entries.len() > 100 {
        return Err(invalid("too many For You entries"));
    }
    let radios = radio_queues(ctx, &[provider_id]).await?;
    let by_id: HashMap<_, _> = radios.iter().map(|q| (q.id, q.clone())).collect();
    let txn = ctx.db.begin().await?;
    let resolver =
        qcm_core::db::sync_batch::ProviderWriter::new(&ctx.db, provider_id, &ctx.sync_coordinator);
    let mut result = wire::HomeRecommendation {
        provider_id,
        epoch,
        ..Default::default()
    };
    let mut keys = HashSet::new();
    let mut placed = HashSet::new();
    let mut targets = HashMap::new();
    for entry in entries {
        if entry.entry_key.is_empty()
            || entry.entry_key.len() > 256
            || !keys.insert(entry.entry_key.clone())
        {
            return Err(invalid("duplicate or invalid For You entry_key"));
        }
        let kind = match &entry.target {
            qcm_core::db::sync_batch::ItemRef::Key(key) => Some(key.r#type),
            _ => None,
        };
        let validate = |kind| -> Result<(), ProcessError> {
            if !entry.primary_action.supports(kind)
                || entry.play_action.is_some_and(|a| !a.supports(kind))
            {
                return Err(invalid(format!(
                    "{}: invalid For You action",
                    entry.entry_key
                )));
            }
            if (entry.presentation.style == CardStyle::Radio
                || matches!(entry.presentation.cover, CardCover::RadioCurrent))
                && kind != ItemType::Radio
            {
                return Err(invalid(format!(
                    "{}: radio presentation requires Radio target",
                    entry.entry_key
                )));
            }
            Ok(())
        };
        if let Some(kind) = kind {
            validate(kind)?;
        }
        let Some(item) = resolver.resolve_optional(&txn, &entry.target, &[]).await? else {
            result.diagnostics.push(format!(
                "{}: missing target {:?}",
                entry.entry_key, entry.target
            ));
            continue;
        };
        validate(item.r#type)?;
        if !targets.contains_key(&item.id) {
            targets.insert(item.id, target(&txn, &item, &by_id).await?);
        }
        let Some(target) = targets[&item.id].clone() else {
            result
                .diagnostics
                .push(format!("{}: missing target metadata", entry.entry_key));
            continue;
        };
        let (kind, id, name) = identity(&target);
        let mut cover = wire::ImageReference {
            r#type: kind as i32,
            id,
        };
        if let CardCover::Item(reference) = &entry.presentation.cover {
            let Some(image) = resolver.resolve_optional(&txn, reference, &[]).await? else {
                result
                    .diagnostics
                    .push(format!("{}: missing cover", entry.entry_key));
                continue;
            };
            if !targets.contains_key(&image.id) {
                targets.insert(image.id, self::target(&txn, &image, &by_id).await?);
            }
            let Some(image) = &targets[&image.id] else {
                result
                    .diagnostics
                    .push(format!("{}: missing cover metadata", entry.entry_key));
                continue;
            };
            let (kind, id, _) = identity(image);
            cover = wire::ImageReference {
                r#type: kind as i32,
                id,
            };
        }
        let card = wire::ForYouCard {
            provider_id,
            entry_key: entry.entry_key,
            title: entry.presentation.title.unwrap_or_else(|| name.to_owned()),
            subtitle: entry.presentation.subtitle,
            badge: entry.presentation.badge.unwrap_or_default(),
            style: match entry.presentation.style {
                CardStyle::Media => wire::CardStyle::Media,
                CardStyle::Radio => wire::CardStyle::Radio,
            } as i32,
            cover_kind: if matches!(entry.presentation.cover, CardCover::RadioCurrent) {
                wire::CardCoverKind::RadioCurrent
            } else {
                wire::CardCoverKind::Item
            } as i32,
            cover: Some(cover),
            primary_action: action(entry.primary_action),
            play_action: entry.play_action.map(action).unwrap_or_default(),
            target: Some(target),
        };
        if kind == ItemType::Radio {
            placed.insert(id);
        }
        result.for_you.push(card);
    }
    txn.commit().await?;
    result.radios = radios
        .into_iter()
        .filter(|q| !placed.contains(&q.id))
        .collect();
    if ctx.sync_coordinator.read_revision(provider_id).await != revision
        || !global::provider_snapshot(provider_id)
            .is_some_and(|(current, stamp)| stamp == epoch && Arc::ptr_eq(&current, &provider))
    {
        return Err(invalid(
            "provider changed while loading home recommendations",
        ));
    }
    Ok(result)
}

pub async fn recommendations(ctx: &Context, ids: &[i64]) -> msg::GetHomeRecommendationsRsp {
    let mut providers = Vec::new();
    let mut seen = HashSet::new();
    for &provider_id in ids {
        if !seen.insert(provider_id) {
            continue;
        }
        let snapshot = global::provider_snapshot(provider_id);
        let epoch = snapshot.as_ref().map_or(0, |(_, epoch)| *epoch);
        let provider = snapshot.map(|(provider, _)| provider);
        let result = if let Some(provider) = provider {
            candidate(ctx, provider_id, provider, epoch).await
        } else {
            Err(ProcessError::NoSuchProvider(provider_id.to_string()))
        };
        providers.push(result.unwrap_or_else(|error| {
            log::warn!("home provider {provider_id}: {error}");
            wire::HomeRecommendation {
                provider_id,
                epoch,
                error: error.to_string(),
                ..Default::default()
            }
        }));
    }
    msg::GetHomeRecommendationsRsp { providers }
}

#[cfg(test)]
mod tests;
