use crate::{
    error::ProcessError,
    msg,
    provider_item::{resolve_provider_item, ProviderItem},
};
use qcm_core::{
    error::ProviderError,
    global,
    provider::{self as p, Context},
};
use serde::{Deserialize, Serialize};
use std::sync::{Arc, LazyLock};

static INSTANCE: LazyLock<String> = LazyLock::new(|| uuid::Uuid::new_v4().to_string());

fn scope(target: &ProviderItem) -> Result<String, ProcessError> {
    let (current, epoch) =
        global::provider_snapshot(target.item.provider_id).ok_or(ProcessError::NotAuth)?;
    if !Arc::ptr_eq(&current, &target.provider) {
        return Err(ProcessError::NotAuth);
    }
    let identity = serde_json::to_vec(&(
        INSTANCE.as_str(),
        epoch,
        target.item.provider_id,
        target.item.id,
        &target.item.native_id,
        target.item.r#type as i32,
    ))
    .map_err(anyhow::Error::from)?;
    let digest = qcm_core::crypto::digest(qcm_core::crypto::MessageDigest::sha256(), &identity)
        .map_err(|err| ProcessError::Internal(anyhow::anyhow!(err.to_string())))?;
    Ok(qcm_core::crypto::encode_block(&digest))
}
fn check_scope(target: &ProviderItem, expected: &str) -> Result<(), ProcessError> {
    if expected.is_empty() || scope(target)? != expected {
        return Err(ProcessError::NotAuth);
    }
    Ok(())
}
fn error(error: ProviderError) -> ProcessError {
    let mut cause = &error;
    while let ProviderError::WithContext { err, .. } = cause {
        cause = err;
    }
    if matches!(cause, ProviderError::NotImplemented) {
        ProcessError::NotImplemented
    } else {
        error.into()
    }
}
fn order(value: i32) -> Result<p::CommentOrder, ProcessError> {
    match msg::model::CommentOrder::try_from(value) {
        Ok(msg::model::CommentOrder::Latest) => Ok(p::CommentOrder::Latest),
        Ok(msg::model::CommentOrder::Hot) => Ok(p::CommentOrder::Hot),
        _ => Err(ProcessError::MissingFields("comment order".into())),
    }
}
fn wire_order(value: p::CommentOrder) -> i32 {
    match value {
        p::CommentOrder::Hot => 1,
        p::CommentOrder::Latest => 0,
    }
}
fn author(value: p::CommentAuthor) -> msg::model::CommentAuthor {
    msg::model::CommentAuthor {
        id: value.id,
        name: value.name,
        avatar_url: value.avatar_url,
    }
}
fn comment(value: p::Comment) -> msg::model::Comment {
    msg::model::Comment {
        id: value.id,
        author: Some(author(value.author)),
        content: value.content,
        time_ms: value.time_ms,
        liked: value.liked,
        like_count: value.like_count,
        reply_count: value.reply_count,
        deleted: value.deleted,
        quote: value.quote.map(|value| msg::model::CommentQuote {
            id: value.id,
            author: Some(author(value.author)),
            content: value.content,
            deleted: value.deleted,
        }),
        previews: value.previews.into_iter().map(comment).collect(),
    }
}
#[derive(Serialize, Deserialize)]
struct Token {
    scope: String,
    id: i64,
    kind: i32,
    order: i32,
    parent: Option<String>,
    limit: u32,
    state: String,
}
pub async fn list(
    ctx: &Context,
    req: &msg::GetCommentsReq,
    parent: Option<&str>,
    expected: Option<&str>,
) -> Result<msg::model::CommentsResult, ProcessError> {
    if !(1..=50).contains(&req.limit)
        || parent.is_some_and(|value| value.is_empty() || value.len() > 256)
    {
        return Err(ProcessError::MissingFields(
            "comment page parameters".into(),
        ));
    }
    let requested_order = order(req.order)?;
    let Some(target) = resolve_provider_item(ctx, req.id, req.item_type).await? else {
        return Ok(Default::default());
    };
    let scope = scope(&target)?;
    if let Some(expected) = expected {
        check_scope(&target, expected)?;
    }
    let state = if let Some(token) = &req.token {
        if token.len() > 16384 {
            return Err(ProcessError::MissingFields("comment token".into()));
        }
        let token: Token = serde_json::from_str(token)
            .map_err(|_| ProcessError::MissingFields("comment token".into()))?;
        if token.scope != scope
            || token.id != req.id
            || token.kind != req.item_type
            || token.order != req.order
            || token.parent.as_deref() != parent
            || token.limit != req.limit
        {
            return Err(ProcessError::MissingFields(
                "expired or mismatched comment token".into(),
            ));
        }
        Some(token.state)
    } else {
        None
    };
    let request = p::CommentPageRequest {
        order: requested_order,
        state,
        limit: req.limit,
    };
    let remote = p::CommentTarget {
        native_id: target.item.native_id.clone(),
        item_type: target.item.r#type,
    };
    let result = if let Some(parent) = parent {
        target
            .provider
            .comment_replies(ctx, remote, parent, request.clone())
            .await
    } else {
        target.provider.comments(ctx, remote, request.clone()).await
    }
    .map_err(error)?;
    check_scope(&target, &scope)?;
    let Some(page) = result else {
        return Ok(Default::default());
    };
    page.validate(&request).map_err(error)?;
    let next_token = page
        .next_state
        .map(|state| {
            serde_json::to_string(&Token {
                scope: scope.clone(),
                id: req.id,
                kind: req.item_type,
                order: req.order,
                parent: parent.map(str::to_owned),
                limit: req.limit,
                state,
            })
        })
        .transpose()
        .map_err(anyhow::Error::from)?;
    Ok(msg::model::CommentsResult {
        supported: true,
        scope,
        provider_id: target.item.provider_id,
        epoch: global::provider_snapshot(target.item.provider_id)
            .ok_or(ProcessError::NotAuth)?
            .1,
        page: Some(msg::model::CommentPage {
            comments: page.comments.into_iter().map(comment).collect(),
            order: wire_order(page.order),
            orders: page.orders.into_iter().map(wire_order).collect(),
            total: page.total,
            has_more: page.has_more,
            next_token,
            can_reply: page.can_reply,
            can_like: page.can_like,
        }),
    })
}

pub async fn like(
    ctx: &Context,
    req: &msg::SetCommentLikeReq,
) -> Result<msg::SetCommentLikeRsp, ProcessError> {
    if req.comment_id.is_empty() || req.comment_id.len() > 256 {
        return Err(ProcessError::MissingFields("comment id".into()));
    }
    let target = resolve_provider_item(ctx, req.id, req.item_type)
        .await?
        .ok_or(ProcessError::NotImplemented)?;
    check_scope(&target, &req.scope)?;
    let result = target
        .provider
        .set_comment_like(
            ctx,
            p::CommentTarget {
                native_id: target.item.native_id.clone(),
                item_type: target.item.r#type,
            },
            &req.comment_id,
            req.value,
        )
        .await
        .map_err(error)?;
    check_scope(&target, &req.scope)?;
    if result.liked != req.value {
        return Err(ProcessError::Internal(anyhow::anyhow!(
            "unexpected comment reaction"
        )));
    }
    Ok(msg::SetCommentLikeRsp {
        result: Some(msg::model::CommentLikeResult {
            liked: result.liked,
            like_count: result.like_count,
        }),
    })
}

#[cfg(test)]
mod tests;
