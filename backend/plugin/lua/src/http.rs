use mlua::prelude::*;
use mlua_extra::http::{CookieHeaderProvider, LuaHttpClient};
pub use mlua_extra::http::{LuaRequestBuilder, LuaResponse};
use qcm_core::http::{BatchRequest, CookieStoreTrait, HeaderValue, HttpClient};
use std::sync::Arc;

struct QcmCookieProvider(Arc<dyn CookieStoreTrait>);

impl CookieHeaderProvider for QcmCookieProvider {
    fn cookies(&self, url: &reqwest::Url) -> Option<HeaderValue> {
        self.0.cookies(url)
    }
}

pub struct LuaClient(LuaHttpClient);

impl LuaClient {
    pub fn new(client: HttpClient, jar: Arc<dyn CookieStoreTrait>) -> Self {
        Self(LuaHttpClient::with_cookie_provider(
            client,
            Arc::new(QcmCookieProvider(jar)),
        ))
    }
}

impl LuaUserData for LuaClient {
    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
        methods.add_method("get", |_, this, url: String| Ok(this.0.get(url)));
        methods.add_method("post", |_, this, url: String| Ok(this.0.post(url)));
        methods.add_method("put", |_, this, url: String| Ok(this.0.put(url)));
        methods.add_method("delete", |_, this, url: String| Ok(this.0.delete(url)));
        methods.add_method("new_batch", |_, _, (): ()| {
            Ok(LuaBatchRequest(BatchRequest::new()))
        });
    }
}

pub struct LuaBatchRequest(BatchRequest);

impl LuaUserData for LuaBatchRequest {
    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
        methods.add_async_method("add", |_, this, ud: LuaAnyUserData| async move {
            let mut request = ud.borrow_mut::<LuaRequestBuilder>()?;
            let (client, req) = request.build_split()?;
            Ok(this.0.add(req, client).await)
        });

        methods.add_async_method("add_rsp", |_, this, ud: LuaAnyUserData| async move {
            let mut response = ud.borrow_mut::<LuaResponse>()?;
            Ok(this
                .0
                .add_rsp(response.0.take().ok_or(mlua::Error::UserDataBorrowError)?)
                .await)
        });

        methods.add_async_method("wait_one", |lua, this, ()| async move {
            match this.0.wait_one().await {
                Some(Ok(bytes)) => Ok(Some(lua.create_string(bytes)?)),
                Some(Err(err)) => Err(mlua::Error::external(err)),
                None => Ok(None),
            }
        });
    }
}
