use crate::{error::ProcessError, msg};
use qcm_core::provider::Context;
#[cfg(test)]
use qcm_core::{global, model as m, model::type_enum::ItemType};

pub async fn link(
    ctx: &Context,
    request: &msg::GetShareLinkReq,
) -> Result<msg::GetShareLinkRsp, ProcessError> {
    let Some(target) =
        crate::provider_item::resolve_provider_item(ctx, request.id, request.item_type).await?
    else {
        return Ok(msg::GetShareLinkRsp::default());
    };
    let url = target
        .provider
        .share_url(&target.item.native_id, target.item.r#type)?;
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
