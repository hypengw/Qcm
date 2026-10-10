use prost;

use super::http::error::HttpError;
use hyper;
use qcm_core::anyhow;
use sea_orm::error as orm_error;
use thiserror::Error;
use tokio::sync::mpsc::error::SendError;

#[derive(Debug, Error, Default)]
pub enum ProcessError {
    #[error("{0}")]
    Resource(anyhow::Error),
    #[error("Not auth")]
    NotAuth,
    #[error("{0}")]
    Internal(#[from] anyhow::Error),
    #[error("Encode error: {0}")]
    Encode(#[from] prost::EncodeError),
    #[error("Decode error: {0}")]
    Decode(#[from] prost::DecodeError),
    #[error("Unsupported message type: {0}")]
    UnsupportedMessageType(i32),
    #[error("Unknown message type: {0}")]
    UnknownMessageType(i32),
    #[error("Unexpected payload for message type: {0}")]
    UnexpectedPayload(i32),
    #[error("Missing fields: {0}")]
    MissingFields(String),
    #[error("No such provider type: {0}")]
    NoSuchProviderType(String),
    #[error("Database error: {0}")]
    Db(#[from] orm_error::DbErr),
    #[error("Hyper body error: {0}")]
    HyperBody(#[from] hyper::Error),
    #[error("Wrong id: {0}")]
    WrongId(String),
    #[error("No such library: {0}")]
    NoSuchLibrary(String),
    #[error("No such provider: {0}")]
    NoSuchProvider(String),
    #[error("No such album: {0}")]
    NoSuchAlbum(String),
    #[error("No such song: {0}")]
    NoSuchSong(String),
    #[error("No such artist: {0}")]
    NoSuchArtist(String),
    #[error("No such mix: {0}")]
    NoSuchMix(String),
    #[error("No such item type: {0}")]
    NoSuchItemType(String),
    #[error("No such image type: {0}")]
    NoSuchImageType(String),
    #[error("No such search type: {0}")]
    NoSuchSearchType(String),
    #[error("Unsupported item type: {0}")]
    UnsupportedItemType(String),
    #[error("parse error: {0}")]
    ParseSubtitle(String),
    #[error("Not Found")]
    NotFound,
    #[error("Not Implemented")]
    NotImplemented,
    #[error("Infallible")]
    Infallible(#[from] std::convert::Infallible),
    #[default]
    #[error("")]
    None,
}

impl<T> From<SendError<T>> for ProcessError
where
    T: Send + Sync + 'static,
{
    fn from(e: SendError<T>) -> Self {
        ProcessError::Internal(e.into())
    }
}

impl From<qcm_core::error::ProviderError> for ProcessError {
    fn from(e: qcm_core::error::ProviderError) -> Self {
        use qcm_core::error::ProviderError;
        let mut cause = &e;
        while let ProviderError::WithContext { err, .. } = cause {
            cause = err;
        }
        if matches!(e, ProviderError::WithContext { .. }) {
            if matches!(cause, ProviderError::NotFound) {
                log::debug!("{e}");
            } else {
                log::error!("{e}");
            }
        }
        match cause {
            ProviderError::Infallible(e) => ProcessError::Infallible(*e),
            ProviderError::NotFound => ProcessError::NotFound,
            ProviderError::NotAuth => ProcessError::NotAuth,
            ProviderError::ParseSubtitle(message) => ProcessError::ParseSubtitle(message.clone()),
            _ => ProcessError::Internal(e.into()),
        }
    }
}

impl From<HttpError> for ProcessError {
    fn from(e: HttpError) -> Self {
        match e {
            HttpError::Cache(e) => ProcessError::Internal(anyhow!(e)),
            HttpError::UnsupportedRange(e) => ProcessError::Internal(anyhow!("{}", e)),
            HttpError::Reqwest(e) => ProcessError::Internal(e.into()),
            HttpError::HyperBody(e) => ProcessError::HyperBody(e),
            HttpError::Infallible(e) => ProcessError::Infallible(e),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use qcm_core::error::ProviderError;

    #[test]
    fn test_provider_error_not_found_to_process_error() {
        let pe = ProviderError::NotFound;
        let converted: ProcessError = pe.into();
        assert!(matches!(converted, ProcessError::NotFound));
    }

    #[test]
    fn test_provider_error_parse_subtitle_to_process_error() {
        let pe = ProviderError::ParseSubtitle("bad lrc".into());
        let converted: ProcessError = pe.into();
        match converted {
            ProcessError::ParseSubtitle(msg) => assert_eq!(msg, "bad lrc"),
            _ => panic!("Expected ParseSubtitle"),
        }
    }

    #[test]
    fn test_provider_error_not_auth_to_process_error() {
        let pe = ProviderError::NotAuth;
        let converted: ProcessError = pe.into();
        assert!(matches!(converted, ProcessError::NotAuth));
    }

    #[test]
    fn test_provider_error_io_to_process_error() {
        let pe = ProviderError::IO(std::io::Error::new(std::io::ErrorKind::NotFound, "file"));
        let converted: ProcessError = pe.into();
        assert!(matches!(converted, ProcessError::Internal(_)));
    }

    #[test]
    fn test_provider_error_not_implemented_maps_to_internal() {
        // NotImplemented from ProviderError doesn't have a specific mapping
        // so it goes to Internal
        let pe = ProviderError::NotImplemented;
        let converted: ProcessError = pe.into();
        assert!(matches!(converted, ProcessError::Internal(_)));
    }

    #[test]
    fn test_provider_error_with_context_to_internal() {
        use std::sync::Arc;
        let inner = ProviderError::Lua("unknown failure".into());
        let pe = ProviderError::WithContext {
            context: "during sync".into(),
            err: Arc::new(inner),
        };
        let converted: ProcessError = pe.into();
        match converted {
            ProcessError::Internal(err) => {
                assert!(err.to_string().contains("during sync"));
                assert!(err.to_string().contains("unknown failure"));
            }
            _ => panic!("Expected Internal"),
        }
    }

    #[test]
    fn test_provider_error_with_context_not_found() {
        use std::sync::Arc;
        let mut error = Arc::new(ProviderError::NotFound);
        for _ in 0..3 {
            let wrapped = ProviderError::WithContext {
                context: "Lua traceback".into(),
                err: error.clone(),
            };
            let converted: ProcessError = wrapped.into();
            assert!(matches!(converted, ProcessError::NotFound));
            error = Arc::new(ProviderError::WithContext {
                context: "resource request".into(),
                err: error,
            });
        }
    }

    #[test]
    fn test_provider_error_with_context_parse_subtitle() {
        let converted: ProcessError = ProviderError::WithContext {
            context: "Lua traceback".into(),
            err: std::sync::Arc::new(ProviderError::ParseSubtitle("bad lrc".into())),
        }
        .into();
        assert!(matches!(converted, ProcessError::ParseSubtitle(message) if message == "bad lrc"));
    }

    #[test]
    fn test_process_error_display() {
        assert_eq!(ProcessError::NotFound.to_string(), "Not Found");
        assert_eq!(ProcessError::NotImplemented.to_string(), "Not Implemented");
        assert_eq!(
            ProcessError::NoSuchAlbum("42".into()).to_string(),
            "No such album: 42"
        );
        assert_eq!(
            ProcessError::MissingFields("name".into()).to_string(),
            "Missing fields: name"
        );
    }

    #[test]
    fn test_process_error_default() {
        let e: ProcessError = Default::default();
        assert!(matches!(e, ProcessError::None));
    }

    #[test]
    fn test_process_error_from_db_err() {
        let db_err = sea_orm::error::DbErr::Custom("test".into());
        let converted: ProcessError = db_err.into();
        assert!(matches!(converted, ProcessError::Db(_)));
    }
}
