use crate::{
    error::ProcessError, event::ServiceContext, http::body_type::ResponseBody,
    resource_cache::ResourceRequest,
};
use hyper::Response;
use qcm_core::{error::ProviderError, model::type_enum::ImageType, resource::Resource};
use reqwest::header::HeaderMap;
use std::sync::Arc;

pub async fn media_get_image(
    ctx: &Arc<ServiceContext>,
    provider_id: i64,
    item_id: &str,
    image_id: Option<&str>,
    image_type: ImageType,
    headers: HeaderMap,
) -> Result<Response<ResponseBody>, ProcessError> {
    resource_response(
        ctx,
        provider_id,
        Resource::Image {
            item_id: item_id.into(),
            image_id: image_id.map(str::to_string),
            kind: image_type,
        },
        headers,
    )
    .await
}
pub async fn media_get_audio(
    ctx: &Arc<ServiceContext>,
    provider_id: i64,
    native_id: &str,
    headers: HeaderMap,
) -> Result<Response<ResponseBody>, ProcessError> {
    resource_response(
        ctx,
        provider_id,
        Resource::Audio {
            item_id: native_id.into(),
        },
        headers,
    )
    .await
}
async fn resource_response(
    ctx: &Arc<ServiceContext>,
    provider_id: i64,
    resource: Resource,
    headers: HeaderMap,
) -> Result<Response<ResponseBody>, ProcessError> {
    let provider = qcm_core::global::provider(provider_id)
        .ok_or(ProcessError::NoSuchProvider(provider_id.to_string()))?;
    let representation = match &resource {
        Resource::Image {
            item_id,
            image_id,
            kind,
        } => format!("image:{item_id:?}:{image_id:?}:{kind}"),
        Resource::Audio { item_id } => format!("audio:{item_id:?}"),
    };
    let kind = if matches!(resource, Resource::Audio { .. }) {
        1
    } else {
        0
    };
    let prepared = match provider.prepare_resource(resource.clone()).await {
        Ok(prepared) => prepared,
        Err(ProviderError::NotImplemented) => {
            let response = match resource {
                Resource::Image {
                    item_id,
                    image_id,
                    kind,
                } => {
                    provider
                        .image(&ctx.provider_context, &item_id, image_id.as_deref(), kind)
                        .await?
                }
                Resource::Audio { item_id } => {
                    provider
                        .audio(&ctx.provider_context, &item_id, Some(headers))
                        .await?
                }
            };
            return ctx.resource_cache.forward(response).await;
        }
        Err(err) => {
            let error: ProcessError = err.into();
            return Err(match error {
                ProcessError::Internal(err) => ProcessError::Resource(err),
                other => other,
            });
        }
    };
    let scope = match provider.resource_scope()? {
        Some(identity) => serde_json::to_string(&(
            provider.type_name(), provider.base_url(), identity, representation,
        )),
        None => serde_json::to_string(&(
            provider.type_name(), provider.base_url(), provider.save(), provider.save_cookie(), representation,
        )),
    }
    .map_err(anyhow::Error::from)?;
    ctx.resource_cache
        .response(ResourceRequest {
            provider: provider_id,
            kind,
            scope,
            prepared: Arc::new(prepared),
            headers,
        })
        .await
}
