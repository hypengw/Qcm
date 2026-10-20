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


use mlua::prelude::*;
use mlua::ObjectLike;

#[derive(Clone)]
pub struct ProviderHttpClient {
    source: Arc<dyn Fn() -> qcm_core::Result<LuaHttpClient> + Send + Sync>,
}
impl ProviderHttpClient {
    pub fn new(source: impl Fn() -> qcm_core::Result<LuaHttpClient> + Send + Sync + 'static) -> Self {
        Self { source: Arc::new(source) }
    }
    fn current(&self) -> LuaResult<LuaHttpClient> { (self.source)().map_err(LuaError::external) }
}
impl LuaUserData for ProviderHttpClient {
    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
        methods.add_method("request", |_, this, (method, url): (String, String)| this.current()?.request(&method, url));
        methods.add_method("get", |_, this, url: String| Ok(this.current()?.get(url)));
        methods.add_method("post", |_, this, url: String| Ok(this.current()?.post(url)));
        methods.add_method("put", |_, this, url: String| Ok(this.current()?.put(url)));
        methods.add_method("delete", |_, this, url: String| Ok(this.current()?.delete(url)));
        methods.add_method("new_batch", |_, _, options: Option<LuaTable>| mlua_extra::http::LuaBatch::new(options));
        methods.add_method("cookie", |_, this, (url, name): (String, String)| this.current()?.cookie(&url, &name));
        methods.add_method("set_cookie", |_, this, (url, cookie): (String, String)| this.current()?.set_cookie(&url, &cookie));
        methods.add_method("clear_cookies", |_, this, ()| { this.current()?.clear_cookies(); Ok(()) });
        methods.add_async_method("send", |lua, this, value: LuaValue| async move {
            let client = lua.create_userdata(this.current()?)?;
            client.call_async_method::<LuaValue>("send", value).await
        });
    }
}
