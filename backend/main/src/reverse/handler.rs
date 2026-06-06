use anyhow::anyhow;
use hyper::Response;
use qcm_core::crypto;
use qcm_core::http;
use qcm_core::model::type_enum::CacheType;
use qcm_core::{model::type_enum::ImageType, Result};
use std::sync::Arc;

use super::connection::Connection;
use super::reverse::{wrap_creator, ReverseEvent};
use crate::error::ProcessError;
use crate::event::ServiceContext;
use crate::http::{
    body_type::ResponseBody,
    range::{parse_range, HttpRange},
};

pub async fn media_get_image(
    ctx: &Arc<ServiceContext>,
    provider_id: i64,
    item_id: &str,
    image_id: Option<&str>,
    image_type: ImageType,
) -> Result<Response<ResponseBody>, ProcessError> {
    let create_rsp = {
        let ctx = ctx.clone();
        let item_id = item_id.to_string();
        let image_id = image_id.map(|s| s.to_string());
        move |_r: Option<HttpRange>| {
            let ctx = ctx.clone();
            let item_id = item_id.clone();
            let image_id = image_id.clone();
            async move {
                let provider = qcm_core::global::provider(provider_id)
                    .ok_or(ProcessError::NoSuchProvider(provider_id.to_string()))?;
                let resp = provider
                    .image(
                        &ctx.provider_context,
                        &item_id,
                        image_id.as_deref(),
                        image_type,
                    )
                    .await?;
                Ok(resp)
            }
        }
    };

    let key = format!(
        "image{}{}{}",
        item_id,
        image_id.unwrap_or_default(),
        image_type
    );
    let key = crypto::digest(crypto::MessageDigest::md5(), key.as_bytes())
        .map(|data| String::from_utf8_lossy(&crypto::hex::encode_low(&data)).to_string())
        .map_err(|_| ProcessError::Internal(anyhow!("md5 error")))?;

    let cnn = Connection::new(&key, None, CacheType::Image);

    let (tx, rx) = tokio::sync::oneshot::channel();
    ctx.reverse_ev
        .send(ReverseEvent::NewConnection(
            cnn,
            wrap_creator(create_rsp),
            tx,
        ))
        .await?;

    let rsp = rx
        .await
        .map_err(|e| ProcessError::Internal(anyhow!("{}", e)))?;
    Ok(rsp?)
}

pub async fn media_get_audio(
    ctx: &Arc<ServiceContext>,
    provider_id: i64,
    native_id: &str,
    headers: http::HeaderMap,
) -> Result<Response<ResponseBody>, ProcessError> {
    let mut headers = headers;
    let range = match headers.get_mut(hyper::header::RANGE) {
        Some(v) => {
            let r = v.to_str().ok().and_then(|r| parse_range(r).ok());
            headers.remove(hyper::header::RANGE);
            r
        }
        None => None,
    };

    let create_rsp = {
        let ctx = ctx.clone();
        let native_id = native_id.to_string();
        let headers = headers.clone();
        move |r: Option<HttpRange>| {
            let ctx = ctx.clone();
            let native_id = native_id.clone();
            let mut headers = headers.clone();
            if let Some(r) = r {
                headers.insert(
                    hyper::header::RANGE,
                    hyper::header::HeaderValue::from_str(&format!("{}", r)).unwrap(),
                );
            }

            async move {
                let provider = qcm_core::global::provider(provider_id)
                    .ok_or(ProcessError::NoSuchProvider(provider_id.to_string()))?;
                let resp = provider
                    .audio(&ctx.provider_context, &native_id, Some(headers))
                    .await?;
                Ok(resp)
            }
        }
    };

    let key = format!("audio{}", native_id);
    let key = crypto::digest(crypto::MessageDigest::md5(), key.as_bytes())
        .map(|data| String::from_utf8_lossy(&crypto::hex::encode_low(&data)).to_string())
        .map_err(|_| ProcessError::Internal(anyhow!("md5 error")))?;

    let cnn = Connection::new(&key, range, CacheType::Audio);

    let (tx, rx) = tokio::sync::oneshot::channel();
    ctx.reverse_ev
        .send(ReverseEvent::NewConnection(
            cnn,
            wrap_creator(create_rsp),
            tx,
        ))
        .await?;

    let rsp = rx
        .await
        .map_err(|e| ProcessError::Internal(anyhow!("{}", e)))?;
    Ok(rsp?)
}
