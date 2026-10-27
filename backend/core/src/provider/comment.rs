use crate::{error::ProviderError, model::type_enum::ItemType};
use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct CommentTarget {
    pub native_id: String,
    pub item_type: ItemType,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Serialize, Deserialize)]
pub enum CommentOrder {
    Hot,
    #[default]
    Latest,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct CommentPageRequest {
    pub order: CommentOrder,
    pub state: Option<String>,
    pub limit: u32,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
#[serde(default)]
pub struct CommentAuthor {
    pub id: String,
    pub name: String,
    pub avatar_url: Option<String>,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
#[serde(default)]
pub struct CommentQuote {
    pub id: String,
    pub author: CommentAuthor,
    pub content: String,
    pub deleted: bool,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
#[serde(default)]
pub struct Comment {
    pub id: String,
    pub author: CommentAuthor,
    pub content: String,
    pub time_ms: Option<i64>,
    pub liked: Option<bool>,
    pub like_count: Option<u64>,
    pub reply_count: Option<u64>,
    pub deleted: bool,
    pub quote: Option<CommentQuote>,
    pub previews: Vec<Comment>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct CommentPage {
    pub comments: Vec<Comment>,
    pub order: CommentOrder,
    pub orders: Vec<CommentOrder>,
    pub total: Option<u64>,
    pub has_more: bool,
    pub next_state: Option<String>,
    /// Allows reading replies, not posting them.
    pub can_reply: bool,
    pub can_like: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct CommentLikeResult {
    pub liked: bool,
    pub like_count: Option<u64>,
}

impl CommentPage {
    pub fn validate(&self, request: &CommentPageRequest) -> Result<(), ProviderError> {
        fn author(value: &CommentAuthor) -> bool {
            value.avatar_url.as_ref().is_none_or(|url| {
                reqwest::Url::parse(url).is_ok_and(|url| {
                    matches!(url.scheme(), "http" | "https")
                        && url.host_str().is_some()
                        && url.username().is_empty()
                        && url.password().is_none()
                })
            })
        }
        fn comment(value: &Comment, depth: usize) -> bool {
            !value.id.is_empty()
                && value.id.len() <= 256
                && author(&value.author)
                && value.time_ms.is_none_or(|time| time >= 0)
                && value
                    .quote
                    .as_ref()
                    .is_none_or(|quote| author(&quote.author))
                && value.previews.len() <= 3
                && (value.previews.is_empty() || depth < 1)
                && value.previews.iter().all(|value| comment(value, depth + 1))
        }
        if self.comments.len() > request.limit as usize
            || self.order != request.order
            || !self.orders.contains(&self.order)
            || self.has_more
                != self
                    .next_state
                    .as_ref()
                    .is_some_and(|value| !value.is_empty())
            || self
                .next_state
                .as_ref()
                .is_some_and(|value| value.len() > 8192 || Some(value) == request.state.as_ref())
            || (self.has_more && self.comments.is_empty())
            || !self.comments.iter().all(|value| comment(value, 0))
        {
            return Err(ProviderError::Internal(crate::anyhow!(
                "invalid comment page"
            )));
        }
        Ok(())
    }
}
