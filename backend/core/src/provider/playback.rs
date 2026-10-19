use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct PlaybackRequest {
    pub item_id: String,
    pub max_bitrate: Option<u32>,
}

#[derive(Clone, Copy, Debug, Deserialize, Serialize, PartialEq)]
#[serde(rename_all = "snake_case")]
pub enum PlaybackKind {
    ByteResource,
    HlsVod,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
pub struct PreparedPlayback {
    pub kind: PlaybackKind,
    pub uri: String,
    pub representation: String,
    pub state: String,
    pub duration_seconds: Option<f64>,
}
