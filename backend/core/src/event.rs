use num_enum::{IntoPrimitive, TryFromPrimitive};
use serde::{Deserialize, Serialize};
use strum_macros::{Display, EnumIter, EnumString};
use tokio::sync::oneshot;

#[derive(
    Copy,
    Debug,
    Clone,
    Default,
    PartialEq,
    Eq,
    Display,
    Serialize,
    Deserialize,
    EnumIter,
    EnumString,
    TryFromPrimitive,
    IntoPrimitive,
)]
#[strum(ascii_case_insensitive)]
#[repr(i32)]
pub enum SyncState {
    #[default]
    Finished = 0,
    Syncing = 1,
    NotAuth = 2,
    NetworkError = 3,
    DBError = 4,
    IOError = 5,
    UknownError = 6,
}

pub enum SyncCommit {
    AddAlbum(i32),
    AddArtist(i32),
    AddSong(i32),
    SetState(SyncState),
}
pub enum Event {
    ProviderSync {
        id: i64,
        oneshot: Option<oneshot::Sender<i64>>,
    },
    SyncCommit {
        id: i64,
        commit: SyncCommit,
    },
    End,
}

fn sync_state_from_provider_error(e: &crate::error::ProviderError) -> SyncState {
    use crate::error::ProviderError;
    match e {
        ProviderError::NotAuth => SyncState::NotAuth,
        ProviderError::Request(_) => SyncState::NetworkError,
        ProviderError::Db(_) => SyncState::DBError,
        ProviderError::IO(_) => SyncState::IOError,
        ProviderError::WithContext { err, .. } => sync_state_from_provider_error(err),
        ProviderError::External(err) => {
            if let Some(p_err) = err.downcast_ref::<ProviderError>() {
                sync_state_from_provider_error(p_err)
            } else if let Some(_) = err.downcast_ref::<sea_orm::DbErr>() {
                SyncState::DBError
            } else if let Some(_) = err.downcast_ref::<std::io::Error>() {
                SyncState::IOError
            } else if let Some(_) = err.downcast_ref::<reqwest::Error>() {
                SyncState::NetworkError
            } else {
                SyncState::UknownError
            }
        }
        _ => SyncState::UknownError,
    }
}

impl From<crate::error::ProviderError> for SyncState {
    fn from(e: crate::error::ProviderError) -> Self {
        sync_state_from_provider_error(&e)
    }
}
