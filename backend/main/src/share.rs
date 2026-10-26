use crate::{error::ProcessError, msg};
use qcm_core::{global, model as m, model::type_enum::ItemType, provider::Context};
use sea_orm::EntityTrait;

pub async fn link(
    ctx: &Context,
    request: &msg::GetShareLinkReq,
) -> Result<msg::GetShareLinkRsp, ProcessError> {
    if request.id <= 0 {
        return Err(ProcessError::WrongId(request.id.to_string()));
    }
    let mut kind = ItemType::try_from(request.item_type)
        .map_err(|_| ProcessError::NoSuchItemType(request.item_type.to_string()))?;
    if kind == ItemType::AlbumArtist {
        kind = ItemType::Artist;
    }
    let item_id = match kind {
        ItemType::Mix => {
            let mix = m::mix::Entity::find_by_id(request.id)
                .one(&ctx.db)
                .await?
                .ok_or_else(|| ProcessError::NoSuchMix(request.id.to_string()))?;
            let Some(remote) = mix.remote_id else {
                return Ok(msg::GetShareLinkRsp::default());
            };
            remote
        }
        ItemType::Song | ItemType::Album | ItemType::Artist => request.id,
        _ => return Ok(msg::GetShareLinkRsp::default()),
    };
    let item = m::item::Entity::find_by_id(item_id)
        .one(&ctx.db)
        .await?
        .ok_or(ProcessError::NotFound)?;
    if item.r#type != kind {
        return Err(ProcessError::WrongId(request.id.to_string()));
    }
    let provider = global::provider(item.provider_id)
        .ok_or_else(|| ProcessError::NoSuchProvider(item.provider_id.to_string()))?;
    let url = provider.share_url(&item.native_id, kind)?;
    if url.as_ref().is_some_and(|url| {
        !matches!(url.scheme(), "http" | "https")
            || url.host_str().is_none()
            || !url.username().is_empty()
            || url.password().is_some()
    }) {
        return Err(ProcessError::Internal(qcm_core::anyhow!(
            "invalid public share URL"
        )));
    }
    Ok(msg::GetShareLinkRsp {
        url: url.map(Into::into),
    })
}

#[cfg(test)]
mod tests;
