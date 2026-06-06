use super::connection::{Connection, Creator, ResponceOneshot};
use crate::error::ProcessError;
use crate::http::range::HttpRange;

pub enum ReverseEvent {
    NewConnection(Connection, Creator, ResponceOneshot),
    Stop,
}

pub fn wrap_creator<Fut>(
    ct: impl Fn(Option<HttpRange>) -> Fut + Send + Sync + 'static,
) -> Creator
where
    Fut: std::future::Future<Output = Result<reqwest::Response, ProcessError>> + Send + 'static,
{
    Box::new(move |r: Option<HttpRange>| Box::pin(ct(r)))
}
