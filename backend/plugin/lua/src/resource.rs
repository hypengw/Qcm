use crate::error::FromLuaError;
use mlua::prelude::*;
use mlua_extra::http::{LuaRequest, LuaRequestBuilder, LuaResponse};
use qcm_core::{error::ProviderError, resource::PreparedResource};

pub struct LuaPreparedResource(pub Option<PreparedResource>);
impl LuaUserData for LuaPreparedResource {}

pub fn take_request(value: &LuaAnyUserData) -> LuaResult<PreparedResource> {
    let (client, request, timeout) = if value.is::<LuaRequest>() {
        value.borrow_mut::<LuaRequest>()?.take_parts()?
    } else {
        value.borrow_mut::<LuaRequestBuilder>()?.take_parts()?
    };
    PreparedResource::new(
        client,
        request,
        timeout.unwrap_or(std::time::Duration::from_secs(30)),
    )
    .map_err(LuaError::external)
}

pub fn take_prepared(value: &LuaAnyUserData) -> LuaResult<PreparedResource> {
    if value.is::<LuaPreparedResource>() {
        value.borrow_mut::<LuaPreparedResource>()?.0.take().ok_or_else(|| LuaError::runtime("resource already consumed"))
    } else { take_request(value) }
}

fn prepare(
    lua: &Lua,
    (request, validator, options): (LuaAnyUserData, LuaFunction, Option<LuaTable>),
) -> LuaResult<LuaPreparedResource> {
    let lua = lua.clone();
    if let Some(options) = &options {
        for pair in options.pairs::<String, LuaValue>() {
            if !matches!(pair?.0.as_str(), "transient_query" | "cache_mode") {
                return Err(LuaError::runtime("unknown resource option"));
            }
        }
    }
    let mode = options.as_ref().map(|o| o.get::<Option<String>>("cache_mode")).transpose()?.flatten();
    let mode = match mode.as_deref() {
        None | Some("byte_range") => qcm_core::resource::CacheMode::ByteRange,
        Some("whole_object") => qcm_core::resource::CacheMode::WholeObject,
        Some("document") => qcm_core::resource::CacheMode::Document,
        _ => return Err(LuaError::runtime("invalid resource cache mode")),
    };
    let fields = options
        .map(|options| options.get::<Option<Vec<String>>>("transient_query"))
        .transpose()?
        .flatten()
        .unwrap_or_default();
    let prepared = take_request(&request)?.with_cache_mode(mode)
        .with_transient_query(fields)
        .map_err(LuaError::external)?
        .validated_by(move |response| {
            let lua = lua.clone();
            let validator = validator.clone();
            Box::pin(async move {
                let response = lua
                    .create_userdata(LuaResponse::from_response(response))
                    .map_err(ProviderError::from_err)?;
                validator
                    .call_async::<()>(response.clone())
                    .await
                    .map_err(ProviderError::from_err)?;
                let result = response
                    .borrow_mut::<LuaResponse>()
                    .and_then(|mut response| response.take_response())
                    .map_err(ProviderError::from_err);
                result
            })
        });
    Ok(LuaPreparedResource(Some(prepared)))
}

pub fn create_module(lua: &Lua) -> LuaResult<LuaTable> {
    let module = lua.create_table()?;
    module.set("prepare", lua.create_function(prepare)?)?;
    Ok(module)
}
