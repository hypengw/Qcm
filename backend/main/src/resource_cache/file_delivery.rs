use super::{format::identity, service::ResourceCache, store::LocalFile};
use crate::http::body_type::ResponseBody;
use anyhow::{ensure, Result};
use hyper::{header::HeaderMap, Method, Response, StatusCode};

pub enum ResourceDelivery {
    Body(Response<ResponseBody>),
    LocalFile(LocalFile),
}
impl ResourceDelivery {
    pub fn into_response(self) -> Result<Response<ResponseBody>> {
        match self {
            Self::Body(response) => Ok(response),
            Self::LocalFile(file) => {
                let url = reqwest::Url::from_file_path(&file.path)
                    .map_err(|_| anyhow::anyhow!("invalid local file URL"))?;
                Ok(Response::builder()
                    .status(StatusCode::FOUND)
                    .header("location", url.as_str())
                    .header("cache-control", "no-store")
                    .header("content-length", "0")
                    .header("qcm-file-lease", &file.token)
                    .header("qcm-file-lease-seconds", file.lifetime.to_string())
                    .header("qcm-file-generation", file.generation)
                    .header(
                        "qcm-file-renew",
                        format!("/resource-file-leases/{}/renew", file.token),
                    )
                    .header(
                        "qcm-file-release",
                        format!("/resource-file-leases/{}", file.token),
                    )
                    .body(ResponseBody::Empty)?)
            }
        }
    }
}
impl ResourceCache {
    /// Enable only after the local client supports file ownership and lease renewal.
    pub fn authorize_file_client(&self, key: &str) -> Result<()> {
        ensure!(
            cfg!(target_os = "linux"),
            "local file delivery is not verified on this platform"
        );
        ensure!(
            key.len() == 32
                && uuid::Uuid::parse_str(key).is_ok_and(|key| key.get_version_num() == 4),
            "invalid file client key"
        );
        *self.file_client.write().unwrap() = Some(identity(key.as_bytes())?);
        Ok(())
    }
    pub(super) fn file_client_for(&self, headers: &HeaderMap) -> Option<String> {
        if headers.get_all("qextra-image-transport").iter().count() != 1
            || headers.get("qextra-image-transport")?.as_bytes() != b"file-lease-v1"
        {
            return None;
        }
        self.authenticated_file_client(headers)
    }
    fn authenticated_file_client(&self, headers: &HeaderMap) -> Option<String> {
        if headers.get_all("qcm-file-client").iter().count() != 1 {
            return None;
        }
        let input = headers.get("qcm-file-client")?.as_bytes();
        if input.len() != 32 {
            return None;
        }
        let hash = identity(input).ok()?;
        self.file_client
            .read()
            .unwrap()
            .as_ref()
            .filter(|key| **key == hash)
            .map(|_| hash)
    }
    pub async fn file_lease_response(
        &self,
        method: &Method,
        path: &str,
        headers: &HeaderMap,
    ) -> Option<Response<ResponseBody>> {
        let rest = path.strip_prefix("/resource-file-leases/")?;
        let response = |status| {
            Response::builder()
                .status(status)
                .header("cache-control", "no-store")
                .body(ResponseBody::Empty)
                .unwrap()
        };
        let Some(client) = self.authenticated_file_client(headers) else {
            return Some(response(StatusCode::FORBIDDEN));
        };
        let (token, renew) = match rest.split_once('/') {
            Some((token, "renew")) => (token, true),
            None => (rest, false),
            _ => return Some(response(StatusCode::NOT_FOUND)),
        };
        if token.len() != 32 || uuid::Uuid::parse_str(token).is_err() {
            return Some(response(StatusCode::NOT_FOUND));
        }
        let Some(store) = &self.store else {
            return Some(response(StatusCode::SERVICE_UNAVAILABLE));
        };
        let result = match (method, renew) {
            (&Method::POST, true) => store.renew_file(token.into(), client).await.map(|ok| {
                if ok {
                    StatusCode::NO_CONTENT
                } else {
                    StatusCode::GONE
                }
            }),
            (&Method::DELETE, false) => store
                .release_file(token.into(), client)
                .await
                .map(|_| StatusCode::NO_CONTENT),
            _ => Ok(StatusCode::METHOD_NOT_ALLOWED),
        };
        let status = result.unwrap_or(StatusCode::SERVICE_UNAVAILABLE);
        let mut reply = response(status);
        if renew && status == StatusCode::NO_CONTENT {
            reply.headers_mut().insert(
                "qcm-file-lease-seconds",
                super::store::LEASE_SECONDS.to_string().parse().unwrap(),
            );
        }
        Some(reply)
    }
}
