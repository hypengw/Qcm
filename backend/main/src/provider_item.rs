use crate::error::ProcessError;
use qcm_core::{
    global, model as m,
    model::type_enum::ItemType,
    provider::{Context, Provider},
};
use sea_orm::EntityTrait;
use std::sync::Arc;

pub struct ProviderItem {
    pub item: m::item::Model,
    pub provider: Arc<dyn Provider>,
}

pub async fn resolve_provider_item(
    ctx: &Context,
    id: i64,
    kind: i32,
) -> Result<Option<ProviderItem>, ProcessError> {
    if id <= 0 {
        return Err(ProcessError::WrongId(id.to_string()));
    }
    let mut kind =
        ItemType::try_from(kind).map_err(|_| ProcessError::NoSuchItemType(kind.to_string()))?;
    if kind == ItemType::AlbumArtist {
        kind = ItemType::Artist;
    }
    let remote = match kind {
        ItemType::Mix => {
            let mix = m::mix::Entity::find_by_id(id)
                .one(&ctx.db)
                .await?
                .ok_or_else(|| ProcessError::NoSuchMix(id.to_string()))?;
            let Some(remote) = mix.remote_id else {
                return Ok(None);
            };
            remote
        }
        ItemType::Song | ItemType::Album | ItemType::Artist => id,
        _ => return Ok(None),
    };
    let item = m::item::Entity::find_by_id(remote)
        .one(&ctx.db)
        .await?
        .ok_or(ProcessError::NotFound)?;
    if item.r#type != kind {
        return Err(ProcessError::WrongId(id.to_string()));
    }
    let provider = global::provider(item.provider_id)
        .ok_or_else(|| ProcessError::NoSuchProvider(item.provider_id.to_string()))?;
    Ok(Some(ProviderItem { item, provider }))
}
