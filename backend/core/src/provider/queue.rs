use super::{Context, Provider, QueueConfigField};
use crate::{
    error::ProviderError,
    model::{song::Model as SongModel, type_enum::ItemType},
};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct QueueDefinition {
    #[serde(default)]
    pub configuration: Vec<QueueConfigField>,
    #[serde(default)]
    pub can_restart: bool,
    #[serde(default)]
    pub restart_label: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct QueueInput {
    pub native_id: String,
    pub item_type: ItemType,
}

#[derive(Clone, Copy, Debug, Default, Serialize, Deserialize)]
pub enum QueueOpenReason {
    #[default]
    Initial,
    Reconfigure,
    Restart,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct QueueOpenRequest {
    pub configuration: HashMap<String, String>,
    pub input: Option<QueueInput>,
    pub reason: QueueOpenReason,
    pub previous_state: Option<String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct QueueNextRequest {
    pub state: String,
    /// Ordered queue contents, not playback history.
    pub current_native_ids: Vec<String>,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct QueuePresentation {
    #[serde(default)]
    pub description: String,
    pub source_item_id: Option<i64>,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct QueueBatch {
    pub songs: Vec<SongModel>,
    /// Provider-owned continuation data, limited to 64 KiB and containing no credentials.
    #[serde(default)]
    pub state: String,
    #[serde(default)]
    pub configuration: HashMap<String, String>,
    #[serde(default)]
    pub presentation: QueuePresentation,
    #[serde(default)]
    pub exhausted: bool,
}

pub async fn legacy_open<P: Provider + ?Sized>(
    provider: &P,
    ctx: &Context,
    native_id: &str,
    request: QueueOpenRequest,
) -> Result<QueueBatch, ProviderError> {
    if request.input.is_some() {
        return Err(ProviderError::NotImplemented);
    }
    let songs = provider
        .get_queue_next_configured(ctx, native_id, &[], &request.configuration)
        .await?;
    Ok(QueueBatch {
        songs,
        state: serde_json::to_string(&request.configuration)?,
        configuration: request.configuration,
        ..Default::default()
    })
}

pub async fn legacy_next<P: Provider + ?Sized>(
    provider: &P,
    ctx: &Context,
    native_id: &str,
    request: QueueNextRequest,
) -> Result<QueueBatch, ProviderError> {
    let configuration = serde_json::from_str(&request.state)?;
    let songs = provider
        .get_queue_next_configured(ctx, native_id, &request.current_native_ids, &configuration)
        .await?;
    Ok(QueueBatch {
        songs,
        state: request.state,
        configuration,
        ..Default::default()
    })
}
