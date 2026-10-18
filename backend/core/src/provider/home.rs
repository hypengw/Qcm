use crate::{db::sync_batch::ItemRef, model::type_enum::ItemType};
use serde::{Deserialize, Serialize};

#[derive(Clone, Copy, Debug, Serialize, Deserialize, PartialEq, Eq)]
pub enum CardAction {
    OpenItem,
    PlayItem,
    ActivateRadio,
}
impl CardAction {
    pub fn supports(self, kind: ItemType) -> bool {
        match self {
            Self::OpenItem => matches!(kind, ItemType::Album | ItemType::Artist | ItemType::Mix),
            Self::PlayItem => matches!(kind, ItemType::Album | ItemType::Song | ItemType::Mix),
            Self::ActivateRadio => kind == ItemType::Radio,
        }
    }
}
#[derive(Clone, Copy, Debug, Default, Serialize, Deserialize, PartialEq, Eq)]
pub enum CardStyle {
    #[default]
    Media,
    Radio,
}
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
#[serde(tag = "type", content = "item", deny_unknown_fields)]
pub enum CardCover {
    #[default]
    Target,
    Item(ItemRef),
    RadioCurrent,
}
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct CardPresentation {
    #[serde(default)]
    pub style: CardStyle,
    pub title: Option<String>,
    pub subtitle: Option<String>,
    pub badge: Option<String>,
    #[serde(default)]
    pub cover: CardCover,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ForYouEntry {
    pub entry_key: String,
    pub target: ItemRef,
    #[serde(default)]
    pub presentation: CardPresentation,
    pub primary_action: CardAction,
    pub play_action: Option<CardAction>,
}
