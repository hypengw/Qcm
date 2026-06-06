use mlua::prelude::*;
use mlua::{Lua, UserData, UserDataMethods};
use qcm_core::http::{BatchRequest, CookieStoreTrait, HttpClient};
use reqwest::header::{self, HeaderName};
use reqwest::{header::HeaderMap, Response};
use serde_json::Value;
use std::collections::HashMap;
use std::str::FromStr;
use std::sync::Arc;

use crate::util::to_lua;

fn header_map_to_table(lua: &Lua, headers: &HeaderMap) -> LuaResult<LuaTable> {
    let table = lua.create_table()?;
    for (name, value) in headers.iter() {
        if let Ok(v) = value.to_str() {
            table.set(name.as_str(), v)?;
        }
    }
    Ok(table)
}

fn table_to_header_map(table: &LuaTable) -> LuaResult<HeaderMap> {
    let mut header_map = HeaderMap::new();
    for pair in table.pairs::<String, String>() {
        let (k, v) = pair?;
        header_map.insert(
            HeaderName::from_str(&k).map_err(mlua::Error::external)?,
            v.parse().map_err(mlua::Error::external)?,
        );
    }
    Ok(header_map)
}

pub struct LuaResponse(pub Option<Response>);

impl UserData for LuaResponse {
    fn add_methods<M: UserDataMethods<Self>>(methods: &mut M) {
        methods.add_async_method_mut("text", |_, mut this, ()| async move {
            this.0
                .take()
                .unwrap()
                .text()
                .await
                .map_err(mlua::Error::external)
        });

        methods.add_async_method_mut("bytes", |lua, mut this, ()| async move {
            this.0
                .take()
                .unwrap()
                .bytes()
                .await
                .map(|b| lua.create_string(b))
                .map_err(mlua::Error::external)
        });

        methods.add_async_method_mut("json", |lua, mut this, ()| async move {
            let value: Value = this
                .0
                .take()
                .unwrap()
                .json()
                .await
                .map_err(mlua::Error::external)?;
            to_lua(&lua, &value)
        });

        methods.add_method("status", |_, this, ()| {
            Ok(this.0.as_ref().unwrap().status().as_u16())
        });

        methods.add_method("headers", |lua, this, ()| {
            let headers = this.0.as_ref().unwrap().headers();
            header_map_to_table(lua, headers)
        });
    }
}

pub struct LuaClient(pub HttpClient, pub Arc<dyn CookieStoreTrait>);

impl UserData for LuaClient {
    fn add_methods<M: UserDataMethods<Self>>(methods: &mut M) {
        methods.add_async_method("get", |_, this, url: String| async move {
            let b = this.0.get(url);
            Ok(LuaRequestBuilder(Some(b), this.1.clone()))
        });

        methods.add_async_method("post", |_, this, url: String| async move {
            let b = this.0.post(url);
            Ok(LuaRequestBuilder(Some(b), this.1.clone()))
        });

        methods.add_async_method("put", |_, this, url: String| async move {
            let b = this.0.put(url);
            Ok(LuaRequestBuilder(Some(b), this.1.clone()))
        });

        methods.add_async_method("delete", |_, this, url: String| async move {
            let b = this.0.delete(url);
            Ok(LuaRequestBuilder(Some(b), this.1.clone()))
        });

        methods.add_method("new_batch", |_, _, (): ()| {
            Ok(LuaBatchRequest(BatchRequest::new()))
        });
    }
}

pub struct LuaRequestBuilder(Option<reqwest::RequestBuilder>, Arc<dyn CookieStoreTrait>);

impl LuaRequestBuilder {
    pub fn merge_cookies(&self, req: &mut reqwest::Request) -> LuaResult<()> {
        let url = req.url().clone();
        let headers = req.headers_mut();
        if let Some(req_cookie) = headers.get_mut(header::COOKIE) {
            if let Some(stored_cookie) = self.1.cookies(&url) {
                let req_cookie_str = req_cookie.to_str().unwrap_or_default();
                let stored_cookie_str = stored_cookie.to_str().unwrap_or_default();
                let merged_cookie = merge_cookies(stored_cookie_str, req_cookie_str);
                *req_cookie = merged_cookie.parse().map_err(mlua::Error::external)?;
            }
        }
        Ok(())
    }

    pub fn build_split(&mut self) -> LuaResult<(HttpClient, reqwest::Request)> {
        let b = self.0.take().ok_or(mlua::Error::UserDataBorrowError)?;
        let (client, req) = b.build_split();
        let mut req = req.map_err(mlua::Error::external)?;
        self.merge_cookies(&mut req)?;
        Ok((client, req))
    }

    pub async fn send(&mut self) -> LuaResult<LuaResponse> {
        let (client, req) = self.build_split()?;
        let response = client.execute(req).await.map_err(|e| {
            log::error!("{:?}", e);
            mlua::Error::external(e)
        })?;
        Ok(LuaResponse(Some(response)))
    }
}

impl UserData for LuaRequestBuilder {
    fn add_methods<M: UserDataMethods<Self>>(methods: &mut M) {
        methods.add_async_method_mut("send", |_, mut this, ()| async move { this.send().await });

        methods.add_function_mut(
            "header",
            |_, (ud, key, value): (LuaAnyUserData, String, String)| {
                let mut this = ud.borrow_mut::<LuaRequestBuilder>()?;
                let b = this
                    .0
                    .take()
                    .ok_or(mlua::Error::UserDataBorrowError)?
                    .header(key, value);
                this.0 = Some(b);
                Ok(ud)
            },
        );

        methods.add_function_mut("headers", |_, (ud, headers): (LuaAnyUserData, LuaTable)| {
            let mut this = ud.borrow_mut::<LuaRequestBuilder>()?;
            let b = this
                .0
                .take()
                .ok_or(mlua::Error::UserDataBorrowError)?
                .headers(table_to_header_map(&headers)?);
            this.0 = Some(b);
            Ok(ud)
        });

        methods.add_function_mut("body", |_, (ud, body): (LuaAnyUserData, LuaValue)| {
            let mut this = ud.borrow_mut::<LuaRequestBuilder>()?;
            this.0 = match body {
                LuaValue::String(s) => {
                    let b = this
                        .0
                        .take()
                        .ok_or(mlua::Error::UserDataBorrowError)?
                        .body(s.to_str()?.to_string());
                    Some(b)
                }
                LuaValue::Table(t) => {
                    let json_str = serde_json::to_string(&t).map_err(mlua::Error::external)?;
                    let b = this
                        .0
                        .take()
                        .ok_or(mlua::Error::UserDataBorrowError)?
                        .header("Content-Type", "application/json")
                        .body(json_str);
                    Some(b)
                }
                _ => return Err(mlua::Error::runtime("Invalid body type")),
            };
            Ok(ud)
        });

        methods.add_function_mut("timeout", |_, (ud, seconds): (LuaAnyUserData, f32)| {
            let mut this = ud.borrow_mut::<LuaRequestBuilder>()?;
            let b = this
                .0
                .take()
                .ok_or(mlua::Error::UserDataBorrowError)?
                .timeout(std::time::Duration::from_secs_f32(seconds));
            this.0 = Some(b);
            Ok(ud)
        });

        methods.add_function_mut("query", |_, (ud, query): (LuaAnyUserData, LuaTable)| {
            let mut this = ud.borrow_mut::<LuaRequestBuilder>()?;
            let b = this
                .0
                .take()
                .ok_or(mlua::Error::UserDataBorrowError)?
                .query(&query);
            this.0 = Some(b);
            Ok(ud)
        });

        methods.add_function_mut("version", |_, (ud, version): (LuaAnyUserData, String)| {
            let mut this = ud.borrow_mut::<LuaRequestBuilder>()?;
            let version = match version.as_str() {
                "HTTP/1.1" => reqwest::Version::HTTP_11,
                "HTTP/2" => reqwest::Version::HTTP_2,
                _ => return Err(mlua::Error::runtime("Unsupported HTTP version")),
            };
            let b = this
                .0
                .take()
                .ok_or(mlua::Error::UserDataBorrowError)?
                .version(version);
            this.0 = Some(b);
            Ok(ud)
        });

        methods.add_function_mut("json", |_, (ud, json): (LuaAnyUserData, LuaValue)| {
            let mut this = ud.borrow_mut::<LuaRequestBuilder>()?;
            let b = this
                .0
                .take()
                .ok_or(mlua::Error::UserDataBorrowError)?
                .json(&json); //.map_err(mlua::Error::external)?;
            this.0 = Some(b);
            Ok(ud)
        });

        methods.add_function_mut("form", |_, (ud, table): (LuaAnyUserData, LuaTable)| {
            let mut this = ud.borrow_mut::<LuaRequestBuilder>()?;
            let b = this
                .0
                .take()
                .ok_or(mlua::Error::UserDataBorrowError)?
                .form(&table);
            this.0 = Some(b);
            Ok(ud)
        });

        methods.add_function_mut("build", |_, ud: LuaAnyUserData| {
            let mut this = ud.borrow_mut::<LuaRequestBuilder>()?;
            let req = this
                .0
                .take()
                .ok_or(mlua::Error::UserDataBorrowError)?
                .build()
                .map_err(mlua::Error::external)?;
            Ok(LuaRequest(req))
        });
    }
}

pub struct LuaRequest(reqwest::Request);

impl UserData for LuaRequest {
    fn add_methods<M: UserDataMethods<Self>>(methods: &mut M) {}
}

fn merge_cookies(stored_cookie: &str, req_cookie_str: &str) -> String {
    let mut cookie_map = HashMap::new();

    // Add stored cookies first
    for cookie in stored_cookie.split(';') {
        if let Some((key, value)) = cookie.trim().split_once('=') {
            cookie_map.insert(key.trim().to_string(), value.trim().to_string());
        }
    }

    // Add request cookies with priority
    for cookie in req_cookie_str.split(';') {
        if let Some((key, value)) = cookie.trim().split_once('=') {
            cookie_map.insert(key.trim().to_string(), value.trim().to_string());
        }
    }

    // Create new cookie string
    cookie_map
        .iter()
        .map(|(k, v)| format!("{}={}", k, v))
        .collect::<Vec<_>>()
        .join("; ")
}

pub struct LuaBatchRequest(BatchRequest);

impl UserData for LuaBatchRequest {
    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
        methods.add_async_method("add", |_, this, ud: LuaAnyUserData| async move {
            let mut rsp = ud.borrow_mut::<LuaRequestBuilder>()?;
            let (client, req) = rsp.build_split()?;
            Ok(this.0.add(req, client).await)
        });

        methods.add_async_method("add_rsp", |_, this, ud: LuaAnyUserData| async move {
            let mut rsp = ud.borrow_mut::<LuaResponse>()?;
            Ok(this
                .0
                .add_rsp(rsp.0.take().ok_or(mlua::Error::UserDataBorrowError)?)
                .await)
        });

        methods.add_async_method("wait_one", |lua, this, ()| async move {
            match this.0.wait_one().await {
                Some(Ok(e)) => Ok(Some(lua.create_string(e)?)),
                Some(Err(e)) => Err(mlua::Error::external(e)),
                None => Ok(None),
            }
        });
    }
}
