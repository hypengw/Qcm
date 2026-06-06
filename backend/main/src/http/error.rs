
use thiserror::Error;

#[derive(Debug, Error)]
pub enum HttpError {
    #[error(transparent)]
    Reqwest(#[from] reqwest::Error),
    #[error("Hyper body error: {0}")]
    HyperBody(#[from] hyper::Error),
    #[error("Unsupported range: {0}")]
    UnsupportedRange(String),
    #[error("Infallible")]
    Infallible(#[from] std::convert::Infallible),
}

