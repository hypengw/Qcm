use crate::error::ProviderError;
use reqwest::{header::HeaderMap, Client, Request, Response};
use std::sync::Arc;
use std::time::Duration;

#[derive(Clone)]
pub enum Resource {
    Image {
        item_id: String,
        image_id: Option<String>,
        kind: crate::model::type_enum::ImageType,
    },
    Audio {
        item_id: String,
    },
}

pub struct PreparedResource {
    client: Client,
    request: Request,
    headers_timeout: Duration,
    valid: Option<Arc<dyn Fn() -> bool + Send + Sync>>,
}

impl PreparedResource {
    /// The client must preserve response encoding and allow streaming without a total timeout.
    pub fn new(
        client: Client,
        mut request: Request,
        headers_timeout: Duration,
    ) -> Result<Self, ProviderError> {
        if request.method() != reqwest::Method::GET || request.body().is_some() {
            return Err(crate::anyhow!("resource request must be a bodyless GET").into());
        }
        request.headers_mut().insert(
            reqwest::header::ACCEPT_ENCODING,
            "identity".parse().unwrap(),
        );
        for name in ["range", "if-range", "if-none-match", "if-modified-since"] {
            request.headers_mut().remove(name);
        }
        Ok(Self {
            client,
            request,
            headers_timeout,
            valid: None,
        })
    }

    pub fn headers(&self) -> &HeaderMap {
        self.request.headers()
    }
    pub fn url(&self) -> &reqwest::Url {
        self.request.url()
    }

    pub async fn fetch(&self, headers: HeaderMap) -> Result<Response, ProviderError> {
        if !self.is_valid() {
            return Err(ProviderError::NotAuth);
        }
        let mut request = self
            .request
            .try_clone()
            .ok_or_else(|| crate::anyhow!("resource request cannot be cloned"))?;
        request.headers_mut().extend(headers);
        let response = tokio::time::timeout(self.headers_timeout, self.client.execute(request))
            .await
            .map_err(|_| crate::anyhow!("resource response headers timed out"))?
            .map_err(|err| ProviderError::Request(err.without_url()))?;
        if !self.is_valid() {
            return Err(ProviderError::NotAuth);
        }
        Ok(response)
    }
    pub fn guarded_by(mut self, valid: impl Fn() -> bool + Send + Sync + 'static) -> Self {
        self.valid = Some(Arc::new(valid));
        self
    }
    pub fn is_valid(&self) -> bool {
        self.valid.as_ref().is_none_or(|valid| valid())
    }
}
