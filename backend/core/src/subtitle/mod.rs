use serde::{Deserialize, Serialize};
mod lrc;

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct SubtitleItem {
    pub start: Option<i64>,
    pub end: Option<i64>,
    pub text: Option<String>,
}

#[derive(Debug, Default, Serialize, Deserialize)]
pub struct Subtitle {
    pub items: Vec<SubtitleItem>,
}

impl Subtitle {
    pub fn from_lrc(lrc: &str) -> Result<Self, lrc::error::Error<&str>> {
        lrc::parse(lrc).map(|lrc_items| {
            let items = lrc_items
                .into_iter()
                .filter_map(|item| match item {
                    lrc::LrcTag::Time(text, timestamps) => {
                        if timestamps.is_empty() || text.is_empty() {
                            None
                        } else {
                            Some(SubtitleItem {
                                start: Some(timestamps[0]),
                                end: timestamps.get(1).cloned(),
                                text: Some(text.to_string()),
                            })
                        }
                    }
                    _ => None,
                })
                .collect();
            Subtitle { items }
        })
    }
}
