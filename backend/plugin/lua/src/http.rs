pub use mlua_extra::http::LuaResponse;
use mlua_extra::http::{CookieHeaderProvider, LuaHttpClient};
use qcm_core::http::{CookieStoreTrait, HeaderValue, HttpClient};
use std::sync::Arc;

struct QcmCookieProvider(Arc<dyn CookieStoreTrait>);
impl CookieHeaderProvider for QcmCookieProvider {
    fn cookies(&self, url: &reqwest::Url) -> Option<HeaderValue> {
        self.0.cookies(url)
    }
}
pub fn new_client(client: HttpClient, jar: Arc<dyn CookieStoreTrait>) -> LuaHttpClient {
    // The core client has no total timeout; both modes share cookies and network policy.
    LuaHttpClient::with_cookie_provider(client.clone(), Arc::new(QcmCookieProvider(jar)))
        .with_stream_client(client)
}
