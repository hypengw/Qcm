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
    #[serde(default)]
    pub uri: String,
    #[serde(default)]
    pub representation: String,
    #[serde(default)]
    pub state: String,
    pub duration_seconds: Option<f64>,
}

impl PreparedPlayback {
    pub fn byte_resource() -> Self {
        Self { kind: PlaybackKind::ByteResource, uri: String::new(), representation: String::new(), state: String::new(), duration_seconds: None }
    }
}
