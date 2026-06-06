use super::process_qcm::process_qcm;
use http_body_util::{BodyExt, Full, Limited};
use hyper::body::{Bytes, Incoming};
use hyper::header::HeaderName;
use hyper::{Request, Response, StatusCode};
use prost::{self, Message};
use qcm_core::http;
use qcm_core::model::type_enum::ImageType;
use qcm_core::{anyhow, model as sqlm, model::type_enum::ItemType, Result};
use sea_orm::{
    sea_query::{Expr, Query},
    EntityTrait, QuerySelect, RelationTrait,
};
use sea_orm::{Condition, QueryFilter};
use std::str::FromStr;
use std::sync::Arc;

use crate::error::ProcessError;
use crate::event::ServiceContext;
use crate::http::body_type::ResponseBody;
use crate::reverse::handler::{media_get_audio, media_get_image};

const SECURE_MAX_SIZE: usize = 64 * 1024;
const HEADER_ICY: HeaderName = HeaderName::from_static("icy-metadata");

pub async fn process_http_post(
    ctx: &Arc<ServiceContext>,
    req: Request<Incoming>,
) -> Result<Response<ResponseBody>, ProcessError> {
    let mut in_id: Option<i32> = None;
    let data = Limited::new(req.into_body(), SECURE_MAX_SIZE)
        .collect()
        .await
        .map_err(|e| ProcessError::Internal(anyhow!("{}", e)))?
        .to_bytes();
    let mut qcm_msg = process_qcm(ctx, &data[..], &mut in_id).await;
    if let (Some(id), Ok(qcm_msg)) = (in_id, &mut qcm_msg) {
        qcm_msg.id = id;
    };

    qcm_msg.and_then(|qcm_msg| -> Result<_, ProcessError> {
        let mut buf = Vec::new();
        qcm_msg.encode(&mut buf)?;
        // let stream = futures_util::stream::once(async {
        //     Ok::<_, std::io::Error>(Frame::data(Bytes::from(buf)))
        // });
        Ok(Response::new(ResponseBody::Boxed(
            Full::new(Bytes::from(buf)).map_err(|e| e.into()).boxed(),
        )))
    })
}

fn filter_image_type(image_type: ImageType) -> Condition {
    Condition::any()
        .add(Expr::col((sqlm::image::Entity, sqlm::image::Column::ImageType)).eq(image_type))
        .add(Expr::col((sqlm::image::Entity, sqlm::image::Column::Id)).is_null())
}

async fn process_http_get_image(
    ctx: &Arc<ServiceContext>,
    item_type: ItemType,
    image_type: ImageType,
    id: i64,
) -> Result<Response<ResponseBody>, ProcessError> {
    let db = &ctx.provider_context.db;

    match item_type {
        ItemType::Album => {
            let (native_id, provider_id, image_native_id): (String, i64, Option<String>) =
                sqlm::album::Entity::find_by_id(id)
                    .inner_join(sqlm::item::Entity)
                    .select_only()
                    .column(sqlm::item::Column::NativeId)
                    .column(sqlm::item::Column::ProviderId)
                    .column(sqlm::image::Column::NativeId)
                    .left_join(sqlm::image::Entity)
                    .filter(filter_image_type(image_type))
                    .into_tuple()
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchAlbum(id.to_string()))?;

            media_get_image(
                ctx,
                provider_id,
                &native_id,
                image_native_id.as_deref(),
                image_type,
            )
            .await
        }
        ItemType::Song => {
            let (mut native_id, provider_id, mut image_native_id): (String, i64, Option<String>) =
                sqlm::song::Entity::find_by_id(id)
                    .inner_join(sqlm::item::Entity)
                    .select_only()
                    .column(sqlm::item::Column::NativeId)
                    .column(sqlm::item::Column::ProviderId)
                    .column(sqlm::image::Column::NativeId)
                    .left_join(sqlm::image::Entity)
                    .filter(filter_image_type(image_type))
                    .into_tuple()
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchSong(id.to_string()))?;

            if image_native_id.is_none() {
                let (album_native_id, album_image_native_id): (Option<String>, Option<String>) =
                    sqlm::album::Entity::find()
                        .inner_join(sqlm::item::Entity)
                        .select_only()
                        .column(sqlm::item::Column::NativeId)
                        .column(sqlm::image::Column::NativeId)
                        .left_join(sqlm::image::Entity)
                        .filter(
                            Expr::col((sqlm::album::Entity, sqlm::album::Column::Id)).in_subquery(
                                Query::select()
                                    .column((sqlm::album::Entity, sqlm::album::Column::Id))
                                    .from(sqlm::song::Entity)
                                    .inner_join(
                                        sqlm::album::Entity,
                                        sqlm::album::Relation::Song.def(),
                                    )
                                    .and_where(
                                        Expr::col((sqlm::song::Entity, sqlm::song::Column::Id))
                                            .eq(id),
                                    )
                                    .to_owned(),
                            ),
                        )
                        .filter(filter_image_type(image_type))
                        .into_tuple()
                        .one(db)
                        .await?
                        .ok_or(ProcessError::NoSuchSong(id.to_string()))?;
                image_native_id = album_image_native_id;
                if let Some(album_native_id) = album_native_id {
                    native_id = album_native_id;
                }
            }

            media_get_image(
                ctx,
                provider_id,
                &native_id,
                image_native_id.as_deref(),
                image_type,
            )
            .await
        }
        ItemType::Artist => {
            let (native_id, provider_id, image_id): (String, i64, Option<String>) =
                sqlm::artist::Entity::find_by_id(id)
                    .inner_join(sqlm::item::Entity)
                    .select_only()
                    .column(sqlm::item::Column::NativeId)
                    .column(sqlm::item::Column::ProviderId)
                    .column(sqlm::image::Column::NativeId)
                    .left_join(sqlm::image::Entity)
                    .filter(filter_image_type(image_type))
                    .into_tuple()
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchArtist(id.to_string()))?;

            media_get_image(
                ctx,
                provider_id,
                &native_id,
                image_id.as_deref(),
                image_type,
            )
            .await
        }
        ItemType::Mix => {
            let (native_id, provider_id, image_id): (String, i64, Option<String>) =
                sqlm::remote_mix::Entity::find()
                    .inner_join(sqlm::item::Entity)
                    .inner_join(sqlm::mix::Entity)
                    .select_only()
                    .column(sqlm::item::Column::NativeId)
                    .column(sqlm::item::Column::ProviderId)
                    .column(sqlm::image::Column::NativeId)
                    .left_join(sqlm::image::Entity)
                    .filter(Expr::col((sqlm::mix::Entity, sqlm::mix::Column::Id)).eq(id))
                    .filter(filter_image_type(image_type))
                    .into_tuple()
                    .one(db)
                    .await?
                    .ok_or(ProcessError::NoSuchMix(id.to_string()))?;

            media_get_image(
                ctx,
                provider_id,
                &native_id,
                image_id.as_deref(),
                image_type,
            )
            .await
        }
        _ => Err(ProcessError::UnsupportedItemType(item_type.to_string())),
    }
}

pub async fn process_http_get(
    ctx: &Arc<ServiceContext>,
    req: Request<Incoming>,
) -> Result<Response<ResponseBody>, ProcessError> {
    let path_segments: Vec<&str> = req.uri().path().split('/').skip(1).collect();

    let parse_id = |id: &str| -> Result<i64, ProcessError> {
        return id
            .parse()
            .map_err(|_| ProcessError::WrongId(id.to_string()));
    };
    let db = &ctx.provider_context.db;

    match path_segments.as_slice() {
        ["image", item_type_str, id, image_type] => {
            let image_type = ImageType::from_str(&image_type)
                .map_err(|_| ProcessError::NoSuchImageType(image_type.to_string()))?;

            let item_type = ItemType::from_str(&item_type_str)
                .map_err(|_| ProcessError::NoSuchItemType(item_type_str.to_string()))?;
            process_http_get_image(ctx, item_type, image_type, parse_id(id)?).await
        }
        ["audio", item_type, id] => {
            match ItemType::from_str(&item_type)
                .map_err(|_| ProcessError::NoSuchItemType(item_type.to_string()))?
            {
                ItemType::Song => {
                    let id = parse_id(id)?;
                    let (native_id, provider_id): (String, i64) =
                        sqlm::song::Entity::find_by_id(id)
                            .inner_join(sqlm::item::Entity)
                            .select_only()
                            .column(sqlm::item::Column::NativeId)
                            .column(sqlm::item::Column::ProviderId)
                            .into_tuple()
                            .one(db)
                            .await?
                            .ok_or(ProcessError::NoSuchSong(id.to_string()))?;

                    let mut headers = http::HeaderMap::new();

                    use reqwest::header;
                    req.headers()
                        .iter()
                        .filter(|(k, _)| match **k {
                            header::ACCEPT => true,
                            header::RANGE => true,
                            header::CONNECTION => true,
                            _ => *k == &HEADER_ICY,
                        })
                        .for_each(|(k, v)| {
                            headers.insert(k, v.clone());
                        });

                    media_get_audio(ctx, provider_id, &native_id, headers).await
                }
                _ => Err(ProcessError::UnsupportedItemType(item_type.to_string())),
            }
        }
        _ => {
            let rsp = Response::builder()
                .status(StatusCode::NOT_FOUND)
                .body(ResponseBody::Empty)
                .unwrap();
            Ok(rsp)
        }
    }
}
