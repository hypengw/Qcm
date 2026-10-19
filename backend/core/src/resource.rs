use crate::error::ProviderError;
use futures::future::BoxFuture;
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
    validate: Option<
        Arc<dyn Fn(Response) -> BoxFuture<'static, Result<Response, ProviderError>> + Send + Sync>,
    >,
    transient_query: Vec<String>,
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
            validate: None,
            transient_query: Vec::new(),
        })
    }

    pub fn headers(&self) -> &HeaderMap {
        self.request.headers()
    }
    pub fn url(&self) -> &reqwest::Url {
        self.request.url()
    }
    pub fn with_transient_query(mut self, names: Vec<String>) -> Result<Self, ProviderError> {
        if names.len() > 32 || names.iter().any(|name| name.is_empty() || name.len() > 128) {
            return Err(crate::anyhow!("invalid transient query fields").into());
        }
        self.transient_query = names;
        Ok(self)
    }
    pub fn cache_url(&self, url: &reqwest::Url) -> reqwest::Url {
        if self.transient_query.is_empty()
            || url.origin() != self.url().origin()
            || url.path() != self.url().path()
        {
            return url.clone();
        }
        let pairs: Vec<_> = url
            .query_pairs()
            .filter(|(name, _)| !self.transient_query.iter().any(|field| field == name))
            .map(|(key, value)| (key.into_owned(), value.into_owned()))
            .collect();
        let mut normalized = url.clone();
        normalized.set_query(None);
        if !pairs.is_empty() {
            normalized.query_pairs_mut().extend_pairs(pairs);
        }
        normalized
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
        let response = if let Some(validate) = &self.validate {
            validate(response).await?
        } else {
            response
        };
        if !self.is_valid() {
            return Err(ProviderError::NotAuth);
        }
        Ok(response)
    }
    pub fn validated_by(
        mut self,
        validate: impl Fn(Response) -> BoxFuture<'static, Result<Response, ProviderError>>
            + Send
            + Sync
            + 'static,
    ) -> Self {
        self.validate = Some(Arc::new(validate));
        self
    }
    pub fn guarded_by(mut self, valid: impl Fn() -> bool + Send + Sync + 'static) -> Self {
        self.valid = Some(Arc::new(valid));
        self
    }
    pub fn is_valid(&self) -> bool {
        self.valid.as_ref().is_none_or(|valid| valid())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn transient_query_preserves_content_and_redirect_identity() {
        let client = Client::new();
        let request = client
            .get("http://localhost/media?id=one&t=nonce")
            .build()
            .unwrap();
        let prepared = PreparedResource::new(client, request, Duration::from_secs(5))
            .unwrap()
            .with_transient_query(vec!["t".into()])
            .unwrap();
        assert_eq!(
            prepared.cache_url(prepared.url()).as_str(),
            "http://localhost/media?id=one"
        );
        let changed = reqwest::Url::parse("http://localhost/media?id=two&t=nonce").unwrap();
        assert_ne!(
            prepared.cache_url(&changed),
            prepared.cache_url(prepared.url())
        );
        for target in [
            "http://cdn/media?id=one&t=content",
            "http://localhost/redirect?id=one&t=content",
        ] {
            let target = reqwest::Url::parse(target).unwrap();
            assert_eq!(prepared.cache_url(&target), target);
        }
    }
}
