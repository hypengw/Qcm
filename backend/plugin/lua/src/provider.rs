use crate::db::{CallGuard, CallState, LuaContext};
use crate::enums;
use crate::error::{create_lua_error_func, FromLuaError};
use mlua::prelude::*;
use mlua_extra::{json, time, url, util::to_lua};
use qcm_core::db::sync::sync_song_album_ids;
use qcm_core::db::values::Timestamp;
use qcm_core::db::{self, DbChunkOper};
use qcm_core::event::{SyncCommit, SyncState};
use qcm_core::model as sqlm;
use qcm_core::provider::{
    AuthResult, ForYouEntry, HasCommonData, HomeBlock, HomeBlockContent, ProviderCommon, ProviderCommonData,
    QrInfo, QueueBatch, QueueConfigField, QueueDefinition, QueueNextRequest, QueueOpenRequest,
};
use qcm_core::{
    anyhow,
    db::DbOper,
    error::ProviderError,
    event::Event as CoreEvent,
    http::{CookieStoreRwLock, HasCookieJar, HeaderMap, HttpClient},
    model::type_enum::ImageType,
    provider::{AuthInfo, Context, Provider},
    subtitle::Subtitle,
    AnyError, Result,
};
use reqwest::Response;
use sea_orm::*;
use std::sync::atomic::{AtomicU64, Ordering};
use std::{collections::HashMap, path::Path, sync::Arc};

use crate::crypto::create_crypto_module;
use crate::http::LuaResponse;
use serde::{Deserialize, Serialize};

struct LuaProviderInner {
    common: ProviderCommonData,
    network: std::sync::RwLock<crate::session::Network>,
    session: crate::session::Session,
    tasks: crate::session::Tasks,
    generation: Arc<AtomicU64>,
}

struct LuaInner(Arc<LuaProviderInner>);

async fn save_session(inner: &LuaProviderInner, lua: &Lua) -> LuaResult<&'static str> {
    let _write = inner.session.writes.lock().await;
    inner.session.check().map_err(LuaError::external)?;
    let save: LuaFunction = lua.named_registry_value("qcm.provider.save")?;
    let custom = save.call::<String>(())?;
    let cookie = qcm_core::http::save_cookie_jar(inner.network.read().unwrap().jar.clone());
    inner
        .session
        .save(inner.common.id(), custom, cookie)
        .await
        .map_err(LuaError::external)
}

impl LuaUserData for LuaInner {
    fn add_fields<F: LuaUserDataFields<Self>>(fields: &mut F) {
        fields.add_field_method_get("id", |_, this| Ok(this.0.common.id()));
        fields.add_field_method_get("base_url", |_, this| Ok(this.0.common.base_url()));
        fields.add_field_method_get("auth_method", |lua, this| to_lua(lua, &this.0.common.auth_method()));
        fields.add_field_method_get("app_name", |_, _| Ok(qcm_core::global::APP_NAME));
        fields.add_field_method_get("app_version", |_, _| Ok(env!("CARGO_PKG_VERSION")));
    }
    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
        methods.add_async_function(
            "session_task",
            |_, (ud, key, callback): (LuaAnyUserData, String, LuaFunction)| async move {
                let inner = ud.borrow::<LuaInner>()?.0.clone();
                inner.session.check().map_err(LuaError::external)?;
                let generation = inner.generation.load(Ordering::Acquire);
                let result = inner.tasks.run(key, callback).await?;
                inner.session.check().map_err(LuaError::external)?;
                if inner.generation.load(Ordering::Acquire) != generation {
                    return Err(LuaError::external(ProviderError::NotAuth));
                }
                Ok(result)
            },
        );
        methods.add_method("new_session_id", |_, _, _: ()| {
            Ok(uuid::Uuid::new_v4().to_string())
        });
        methods.add_method("begin_session", |_, this, _: ()| {
            this.0.session.check().map_err(LuaError::external)?;
            let next = crate::session::Network::new().map_err(LuaError::external)?;
            let mut network = this.0.network.write().unwrap();
            network.invalidate();
            *network = next;
            this.0.tasks.cancel();
            this.0.generation.fetch_add(1, Ordering::AcqRel);
            Ok(())
        });
        methods.add_async_function("save_session", |lua, ud: LuaAnyUserData| async move {
            let inner = ud.borrow::<LuaInner>()?.0.clone();
            save_session(&inner, &lua).await
        });
        methods.add_method("device_id", |_, this, _: ()| {
            Ok(this.0.common.device_id.clone())
        });
    }
}

struct LuaImpl {
    load: LuaFunction,
    save: LuaFunction,
    check: LuaFunction,
    login: LuaFunction,
    sync: LuaFunction,
    sync_item: LuaFunction,
    get_queue_next: Option<LuaFunction>,
    queue_configuration: Option<LuaFunction>,
    queue_definition: Option<LuaFunction>,
    open_queue: Option<LuaFunction>,
    next_queue: Option<LuaFunction>,
    close_queue: Option<LuaFunction>,
    favorite: LuaFunction,
    qr: Option<LuaFunction>,
    image: LuaFunction,
    audio: LuaFunction,
    prepare_image: Option<LuaFunction>,
    prepare_audio: Option<LuaFunction>,
    open_playback: Option<LuaFunction>,
    prepare_playback_resource: Option<LuaFunction>,
    keep_playback: Option<LuaFunction>,
    close_playback: Option<LuaFunction>,
    subtitle: LuaFunction,
    for_you_items: Option<LuaFunction>,
    home_blocks: Option<LuaFunction>,
    home_block_items: Option<LuaFunction>,
}

#[derive(Debug, Serialize, Deserialize)]
struct LuaSyncOption {
    #[serde(default)]
    include: Vec<String>,
    #[serde(default)]
    exclude: Vec<String>,
}

impl LuaSyncOption {
    fn generate_exclude<C>(&self) -> Vec<C>
    where
        C: sea_orm::ColumnTrait,
    {
        let mut out: Vec<C> = Vec::new();
        let opt_include: Vec<C> = self
            .include
            .iter()
            .filter_map(|s| C::from_str(&s).ok())
            .collect();
        if !opt_include.is_empty() {
            out.extend(C::iter().filter(|c| !qcm_core::db::columns_contains(&opt_include, c)));
        }
        out.extend(self.exclude.iter().filter_map(|s| C::from_str(&s).ok()));
        out
    }
}

pub struct LuaProvider {
    inner: Arc<LuaProviderInner>,
    funcs: LuaImpl,
    lua: Lua,
    generation: Arc<AtomicU64>,
    capabilities: crate::plugin::Capabilities,
    saved: std::sync::Mutex<String>,
}

impl LuaProvider {
    pub fn new(
        id: Option<i64>,
        name: &str,
        device_id: &str,
        meta_type: &str,
        script_path: &Path,
    ) -> Result<Self> {
        Self::with_capabilities(
            id,
            name,
            device_id,
            meta_type,
            script_path,
            Default::default(),
        )
    }
    pub fn with_capabilities(
        id: Option<i64>,
        name: &str,
        device_id: &str,
        meta_type: &str,
        script_path: &Path,
        capabilities: crate::plugin::Capabilities,
    ) -> Result<Self> {
        capabilities.validate()?;
        let lua = Lua::new();
        let inner = Arc::new(LuaProviderInner {
            common: ProviderCommonData::new(id, name, device_id, meta_type),
            network: std::sync::RwLock::new(crate::session::Network::new()?),
            session: crate::session::Session::new(),
            tasks: Default::default(),
            generation: Arc::new(AtomicU64::new(1)),
        });
        {
            // set package.path
            let package = lua.globals().get::<LuaTable>("package")?;
            package.set(
                "path",
                script_path
                    .parent()
                    .and_then(|p| p.to_str())
                    .map(|p| format!("{}/?.lua", p)),
            )?;

            // qcm table
            let qcm_table = lua.create_table()?;
            qcm_table.set("inner", LuaInner(inner.clone()))?;
            let url_module = url::create_module(&lua)?;
            let crypto_module = create_crypto_module(&lua)?;
            crypto_module.set("url", url_module.clone())?;
            qcm_table.set("crypto", crypto_module)?;
            qcm_table.set("random", mlua_extra::random::create_module(&lua)?)?;
            qcm_table.set("resource", crate::resource::create_module(&lua)?)?;
            qcm_table.set("url", url_module)?;
            qcm_table.set("json", json::create_module(&lua)?)?;
            qcm_table.set("null", LuaValue::NULL)?;
            qcm_table.set("errors", mlua_extra::error::create_module(&lua)?)?;
            qcm_table.set("time", time::create_module(&lua)?)?;
            qcm_table.set("enum", enums::create_module(&lua)?)?;
            qcm_table.set("error", create_lua_error_func(&lua)?)?;

            let inner = inner.clone();
            qcm_table.set(
                "get_http_client",
                lua.create_function(move |_, ()| {
                    Ok(inner.network.read().unwrap().lua_client())
                })?,
            )?;
            qcm_table.set(
                "debug",
                lua.create_function(
                    |_, val: LuaValue| match serde_json::to_string_pretty(&val) {
                        Ok(val) => {
                            log::info!("{}", val);
                            Ok(())
                        }
                        Err(e) => Err(mlua::Error::external(e)),
                    },
                )?,
            )?;
            lua.globals().set("qcm", qcm_table)?;
        }

        // Load the provider script
        let func = lua.load(script_path).into_function()?;
        let provider_table = func.call::<LuaTable>(())?;

        lua.set_named_registry_value("qcm.provider.resource_scope", provider_table.get::<Option<LuaFunction>>("resource_scope")?)?;
        lua.set_named_registry_value("qcm.provider.save", provider_table.get::<LuaFunction>("save")?)?;
        let playback_hooks = ["open_playback", "prepare_playback_resource", "keep_playback", "close_playback"];
        let playback_count = playback_hooks.iter().filter(|name| provider_table.get::<Option<LuaFunction>>(**name).ok().flatten().is_some()).count();
        let playback_required = capabilities.requires.iter().any(|c| c == "playback.lifecycle.v1");
        if (playback_count != 0 || playback_required) && (playback_count != playback_hooks.len() || !playback_required) {
            return Err(anyhow!("playback.lifecycle.v1 requires all playback hooks"));
        }
        let hooks = ["queue_definition", "open_queue", "next_queue"];
        let count = hooks
            .iter()
            .filter(|name| {
                provider_table
                    .get::<Option<LuaFunction>>(**name)
                    .ok()
                    .flatten()
                    .is_some()
            })
            .count();
        let lifecycle = capabilities
            .requires
            .iter()
            .any(|c| c == "radio.lifecycle.v1");
        if (count != 0 && count != hooks.len()) || (lifecycle && count != hooks.len()) {
            return Err(anyhow!(
                "radio.lifecycle.v1 requires queue_definition, open_queue and next_queue together"
            ));
        }
        if count != 0 && !lifecycle {
            return Err(anyhow!("radio lifecycle hooks require radio.lifecycle.v1"));
        }
        if provider_table.contains_key("close_queue")? && count == 0 {
            return Err(anyhow!("close_queue requires radio.lifecycle.v1"));
        }
        let provider = Self {
            inner: inner.clone(),
            funcs: LuaImpl {
                save: provider_table
                    .get::<LuaFunction>("save")
                    .map_err(|_| anyhow!("save func not found"))?,
                load: provider_table
                    .get::<LuaFunction>("load")
                    .map_err(|_| anyhow!("load func not found"))?,
                check: provider_table
                    .get::<LuaFunction>("check")
                    .map_err(|_| anyhow!("check func not found"))?,
                login: provider_table
                    .get::<LuaFunction>("login")
                    .map_err(|_| anyhow!("login func not found"))?,
                sync: provider_table
                    .get::<LuaFunction>("sync")
                    .map_err(|_| anyhow!("sync func not found"))?,
                sync_item: provider_table
                    .get::<LuaFunction>("sync_item")
                    .map_err(|_| anyhow!("sync_item func not found"))?,
                get_queue_next: provider_table.get("get_queue_next")?,
                queue_configuration: provider_table.get("queue_configuration")?,
                queue_definition: provider_table.get("queue_definition")?,
                open_queue: provider_table.get("open_queue")?,
                next_queue: provider_table.get("next_queue")?,
                close_queue: provider_table.get("close_queue")?,
                favorite: provider_table
                    .get::<LuaFunction>("favorite")
                    .map_err(|_| anyhow!("favorite func not found"))?,
                qr: provider_table.get::<LuaFunction>("qr").ok(),
                image: provider_table
                    .get::<LuaFunction>("image")
                    .map_err(|_| anyhow!("image func not found"))?,
                audio: provider_table
                    .get::<LuaFunction>("audio")
                    .map_err(|_| anyhow!("audio func not found"))?,
                prepare_image: provider_table.get("prepare_image")?,
                prepare_audio: provider_table.get("prepare_audio")?,
                open_playback: provider_table.get("open_playback")?,
                prepare_playback_resource: provider_table.get("prepare_playback_resource")?,
                keep_playback: provider_table.get("keep_playback")?,
                close_playback: provider_table.get("close_playback")?,
                subtitle: provider_table
                    .get::<LuaFunction>("subtitle")
                    .map_err(|_| anyhow!("subtitle func not found"))?,
                for_you_items: provider_table.get("for_you_items")?,
                home_blocks: provider_table.get::<LuaFunction>("home_blocks").ok(),
                home_block_items: provider_table.get::<LuaFunction>("home_block_items").ok(),
            },
            lua,
            generation: inner.generation.clone(),
            capabilities,
            saved: std::sync::Mutex::new(String::new()),
        };
        Ok(provider)
    }

    fn context(
        &self,
        ctx: &Context,
        run: Option<qcm_core::db::sync_run::SyncRun>,
    ) -> (LuaContext, CallGuard) {
        let (state, guard) = CallState::new(
            self.generation.clone(),
            self.capabilities.explicit(),
            run,
            self.capabilities.sync_types.clone(),
        );
        (LuaContext(ctx.clone(), self.id(), state), guard)
    }
    pub fn type_name() -> &'static str {
        "lua"
    }

    pub fn client(&self) -> HttpClient {
        return self.inner.network.read().unwrap().client.clone();
    }
}

impl HasCookieJar for LuaProvider {
    fn jar(&self) -> Arc<CookieStoreRwLock> {
        self.inner.network.read().unwrap().jar.clone()
    }
}

impl HasCommonData for LuaProvider {
    fn common<'a>(&'a self) -> &'a ProviderCommonData {
        &self.inner.common
    }
}

#[async_trait::async_trait]
impl Provider for LuaProvider {
    fn lifecycle_epoch(&self) -> u64 {
        self.generation.load(Ordering::Acquire)
    }
    fn invalidate(&self) {
        self.generation.fetch_add(1, Ordering::AcqRel);
        self.inner.session.invalidate();
        self.inner.tasks.cancel();
        self.inner.network.read().unwrap().invalidate();
    }
    async fn persist_session(&self) -> Result<(), ProviderError> {
        if self
            .capabilities
            .requires
            .iter()
            .any(|c| c == "provider.session.v1")
        {
            save_session(&self.inner, &self.lua)
                .await
                .map_err(ProviderError::from_err)?;
        }
        Ok(())
    }
    fn session_key(&self) -> Option<String> {
        Some(self.inner.session.key())
    }
    fn bind_session(&self, db: DatabaseConnection, key: String) {
        self.inner.session.bind(db, key);
    }
    fn resource_scope(&self) -> Result<Option<String>, ProviderError> {
        self.lua
            .named_registry_value::<Option<LuaFunction>>("qcm.provider.resource_scope")
            .and_then(|f| f.map(|f| f.call::<String>(())).transpose())
            .map_err(ProviderError::from_err)
    }
    fn load(&self, data: &str) {
        self.generation.fetch_add(1, Ordering::AcqRel);
        self.inner.tasks.cancel();
        match crate::session::Network::new() {
            Ok(next) => {
                let mut current = self.inner.network.write().unwrap();
                let cookies = qcm_core::http::save_cookie_jar(current.jar.clone());
                qcm_core::http::load_cookie_jar(next.jar.clone(), &cookies);
                current.invalidate();
                *current = next;
            }
            Err(error) => {
                log::error!("Could not restore provider HTTP session: {error}");
                return;
            }
        }
        *self.saved.lock().unwrap() = data.to_owned();
        if let Err(error) = self.funcs.load.call::<()>(data.to_string()) {
            log::error!("Lua provider load failed: {error}");
        }
    }

    fn save(&self) -> String {
        match self.funcs.save.call::<String>(()) {
            Ok(value) => {
                *self.saved.lock().unwrap() = value.clone();
                value
            }
            Err(error) => {
                log::error!("Lua provider save failed: {error}");
                self.saved.lock().unwrap().clone()
            }
        }
    }

    async fn check(&self, _ctx: &Context) -> Result<(), ProviderError> {
        self.funcs
            .check
            .call_async::<()>(())
            .await
            .map_err(ProviderError::from_err)
    }

    async fn auth(&self, _ctx: &Context, info: &AuthInfo) -> Result<AuthResult, ProviderError> {
        self.generation.fetch_add(1, Ordering::AcqRel);
        let res = self
            .funcs
            .login
            .call_async::<LuaValue>(to_lua(&self.lua, info))
            .await
            .and_then(|v| self.lua.from_value(v))
            .map_err(ProviderError::from_err);

        if let Ok(AuthResult::Ok) = &res {
            self.load_auth_info(&info.server_url, info.method.clone());
        }
        res
    }

    async fn sync(&self, ctx: &Context) -> Result<(), ProviderError> {
        let now = chrono::Utc::now();
        let run = if self.capabilities.explicit() {
            Some(
                ctx.sync_coordinator
                    .start(self.id().ok_or(ProviderError::NotAuth)?)
                    .await?,
            )
        } else {
            None
        };
        let (call, _guard) = self.context(ctx, run);
        let pending = call.2.run.clone();
        let res = self
            .funcs
            .sync
            .call_async::<LuaValue>(call.clone())
            .await
            .map_err(ProviderError::from_err)?;

        if res.is_integer() {
            let val = res.as_i32().and_then(|v| SyncState::try_from(v).ok());
            match val {
                Some(SyncState::NotAuth) => return Err(ProviderError::NotAuth),
                _ => {}
            }
        }

        if let Some(run) = pending {
            run.lock()
                .await
                .finalize(&ctx.db, &ctx.sync_coordinator)
                .await?;
            return Ok(());
        }
        if let Some(id) = self.id() {
            let write = ctx.sync_coordinator.write(id, None).await?;
            let txn = ctx.db.begin().await?;
            db::sync::sync_drop_before(&txn, id, now, &write.retained_items()).await?;
            txn.commit().await?;
        }
        Ok(())
    }
    async fn sync_item(&self, ctx: &Context, item: sqlm::item::Model) -> Result<(), ProviderError> {
        let (call, _guard) = self.context(ctx, None);
        self.funcs
            .sync_item
            .call_async::<LuaValue>((call.clone(), to_lua(&self.lua, &item)))
            .await
            .map_err(ProviderError::from_err)?;
        Ok(())
    }

    async fn get_queue_next(
        &self,
        ctx: &Context,
        native_id: &str,
        current_native_ids: &[String],
    ) -> Result<Vec<sqlm::song::Model>, ProviderError> {
        self.get_queue_next_configured(ctx, native_id, current_native_ids, &HashMap::new())
            .await
    }

    async fn queue_configuration(
        &self,
        ctx: &Context,
        native_id: &str,
    ) -> Result<Vec<QueueConfigField>, ProviderError> {
        let Some(function) = &self.funcs.queue_configuration else {
            return Ok(Vec::new());
        };
        let (call, _guard) = self.context(ctx, None);
        let res: LuaValue = function
            .call_async((call, native_id.to_string()))
            .await
            .map_err(ProviderError::from_err)?;
        self.lua.from_value(res).map_err(ProviderError::from_err)
    }

    async fn get_queue_next_configured(
        &self,
        ctx: &Context,
        native_id: &str,
        current_native_ids: &[String],
        configuration: &HashMap<String, String>,
    ) -> Result<Vec<sqlm::song::Model>, ProviderError> {
        let (call, _guard) = self.context(ctx, None);
        let res: LuaValue = self
            .funcs
            .get_queue_next
            .as_ref()
            .ok_or(ProviderError::NotImplemented)?
            .call_async((
                call.clone(),
                native_id.to_string(),
                current_native_ids.to_vec(),
                to_lua(&self.lua, configuration),
            ))
            .await
            .map_err(ProviderError::from_err)?;

        self.lua.from_value(res).map_err(ProviderError::from_err)
    }

    async fn queue_definition(
        &self,
        ctx: &Context,
        native_id: &str,
    ) -> Result<QueueDefinition, ProviderError> {
        let Some(function) = &self.funcs.queue_definition else {
            return Ok(QueueDefinition {
                configuration: self.queue_configuration(ctx, native_id).await?,
                ..Default::default()
            });
        };
        let (call, _guard) = self.context(ctx, None);
        let result = function
            .call_async::<LuaValue>((call, native_id))
            .await
            .map_err(ProviderError::from_err)?;
        self.lua.from_value(result).map_err(ProviderError::from_err)
    }
    async fn open_queue(
        &self,
        ctx: &Context,
        native_id: &str,
        request: QueueOpenRequest,
    ) -> Result<QueueBatch, ProviderError> {
        let Some(function) = &self.funcs.open_queue else {
            return qcm_core::provider::queue::legacy_open(self, ctx, native_id, request).await;
        };
        let (call, _guard) = self.context(ctx, None);
        let result = function
            .call_async::<LuaValue>((call, native_id, to_lua(&self.lua, &request)))
            .await
            .map_err(ProviderError::from_err)?;
        self.lua.from_value(result).map_err(ProviderError::from_err)
    }
    async fn next_queue(
        &self,
        ctx: &Context,
        native_id: &str,
        request: QueueNextRequest,
    ) -> Result<QueueBatch, ProviderError> {
        let Some(function) = &self.funcs.next_queue else {
            return qcm_core::provider::queue::legacy_next(self, ctx, native_id, request).await;
        };
        let (call, _guard) = self.context(ctx, None);
        let result = function
            .call_async::<LuaValue>((call, native_id, to_lua(&self.lua, &request)))
            .await
            .map_err(ProviderError::from_err)?;
        self.lua.from_value(result).map_err(ProviderError::from_err)
    }
    async fn close_queue(
        &self,
        ctx: &Context,
        native_id: &str,
        state: &str,
    ) -> Result<(), ProviderError> {
        if let Some(function) = &self.funcs.close_queue {
            let (call, _guard) = self.context(ctx, None);
            function
                .call_async::<()>((call, native_id, state))
                .await
                .map_err(ProviderError::from_err)?;
        }
        Ok(())
    }

    async fn favorite(
        &self,
        ctx: &Context,
        item_id: &str,
        item_type: sqlm::type_enum::ItemType,
        value: bool,
    ) -> Result<(), ProviderError> {
        let (call, _guard) = self.context(ctx, None);
        let res = self
            .funcs
            .favorite
            .call_async::<LuaValue>((call.clone(), item_id, item_type as i32, value))
            .await
            .map_err(ProviderError::from_err)?;
        Ok(())
    }

    async fn qr(&self, _ctx: &Context) -> Result<QrInfo, ProviderError> {
        let func = self
            .funcs
            .qr
            .as_ref()
            .ok_or(ProviderError::NotImplemented)?;
        func.call_async::<LuaValue>(())
            .await
            .and_then(|v| self.lua.from_value(v))
            .map_err(ProviderError::from_err)
    }

    async fn image(
        &self,
        _ctx: &Context,
        item_id: &str,
        image_id: Option<&str>,
        image_type: ImageType,
    ) -> Result<Response, ProviderError> {
        let val = self
            .funcs
            .image
            .call_async::<LuaValue>((item_id, image_id, image_type.to_string()))
            .await
            .map_err(ProviderError::from_err)?;

        if val.is_userdata() {
            let ud = val.as_userdata().unwrap();
            let mut ud_rsp = ud.borrow_mut::<LuaResponse>().map_err(AnyError::from)?;
            let rsp = ud_rsp.take_response().map_err(ProviderError::from_err)?;
            Ok(rsp)
        } else {
            Err(ProviderError::NotFound)
        }
    }

    async fn open_playback(&self, request: qcm_core::provider::PlaybackRequest) -> Result<qcm_core::provider::PreparedPlayback, ProviderError> {
        let function = self.funcs.open_playback.as_ref().ok_or(ProviderError::NotImplemented)?;
        let epoch = self.generation.load(Ordering::Acquire);
        let value = function.call_async::<LuaValue>(self.lua.to_value(&request).map_err(ProviderError::from_err)?).await.map_err(ProviderError::from_err)?;
        let result = self.lua.from_value(value).map_err(ProviderError::from_err)?;
        if epoch != self.generation.load(Ordering::Acquire) { return Err(ProviderError::NotAuth); }
        Ok(result)
    }
    async fn prepare_playback_resource(&self, state: &str, uri: &str) -> Result<qcm_core::resource::PreparedResource, ProviderError> {
        let epoch = self.generation.load(Ordering::Acquire);
        let function = self.funcs.prepare_playback_resource.as_ref().ok_or(ProviderError::NotImplemented)?;
        let value = function.call_async::<LuaAnyUserData>((state,uri)).await.map_err(ProviderError::from_err)?;
        let prepared = crate::resource::take_prepared(&value).map_err(ProviderError::from_err)?;
        let generation = self.generation.clone();
        if generation.load(Ordering::Acquire) != epoch { return Err(ProviderError::NotAuth); }
        Ok(prepared.guarded_by(move || generation.load(Ordering::Acquire) == epoch))
    }
    async fn keep_playback(&self, state: &str) -> Result<(), ProviderError> {
        if let Some(function) = &self.funcs.keep_playback { function.call_async::<()> (state).await.map_err(ProviderError::from_err)?; }
        Ok(())
    }
    async fn close_playback(&self, state: &str) -> Result<(), ProviderError> {
        if let Some(function) = &self.funcs.close_playback { function.call_async::<()> (state).await.map_err(ProviderError::from_err)?; }
        Ok(())
    }

    async fn prepare_resource(&self, resource: qcm_core::resource::Resource) -> Result<qcm_core::resource::PreparedResource, ProviderError> {
        let generation=self.generation.load(Ordering::Acquire);
        use qcm_core::resource::Resource;
        let value = match resource {
            Resource::Image { item_id, image_id, kind } => {
                let function = self.funcs.prepare_image.as_ref().ok_or(ProviderError::NotImplemented)?;
                function.call_async::<LuaValue>((item_id, image_id, kind.to_string())).await
            }
            Resource::Audio { item_id } => {
                let function = self.funcs.prepare_audio.as_ref().ok_or(ProviderError::NotImplemented)?;
                function.call_async::<LuaValue>(item_id).await
            }
        }.map_err(ProviderError::from_err)?;
        let ud = value.as_userdata().ok_or(ProviderError::NotFound)?;
        let prepared = crate::resource::take_prepared(ud).map_err(ProviderError::from_err)?;
        let revision=self.generation.clone();
        if revision.load(Ordering::Acquire)!=generation {return Err(ProviderError::NotAuth);}
        Ok(prepared.guarded_by(move ||revision.load(Ordering::Acquire)==generation))
    }

    async fn audio(
        &self,
        _ctx: &Context,
        item_id: &str,
        headers: Option<HeaderMap>,
    ) -> Result<Response, ProviderError> {
        let headers = headers.map(|h| {
            let map = self.lua.create_table().unwrap();
            for (k, v) in h.iter() {
                let k = k.as_str();
                let v = v
                    .to_str()
                    .inspect_err(|e| log::error!("{}", e))
                    .unwrap_or("");
                map.set(k, v).unwrap();
            }
            map
        });
        let val = self
            .funcs
            .audio
            .call_async::<LuaValue>((item_id, headers))
            .await
            .map_err(ProviderError::from_err)?;

        if val.is_userdata() {
            let ud = val.as_userdata().unwrap();
            let mut ud_rsp = ud.borrow_mut::<LuaResponse>().map_err(AnyError::from)?;
            let rsp = ud_rsp.take_response().map_err(ProviderError::from_err)?;
            Ok(rsp)
        } else {
            Err(ProviderError::NotFound)
        }
    }
    async fn subtitle(&self, item_id: &str) -> Result<Subtitle, ProviderError> {
        let val = self
            .funcs
            .subtitle
            .call_async::<LuaValue>(item_id)
            .await
            .map_err(ProviderError::from_err)?;
        if val.is_string() {
            let lrc = val.to_string().unwrap();
            let subtitle = Subtitle::from_lrc(&lrc)
                .map_err(|e| ProviderError::ParseSubtitle(format!("{:?}", e)))?;
            Ok(subtitle)
        } else if val.is_table() {
            let subtitle: Subtitle = self.lua.from_value(val).map_err(ProviderError::from_err)?;
            if subtitle.items.len() > 100_000
                || subtitle.items.iter().any(|v| {
                    v.start.is_some_and(|t| t < 0)
                        || v.end.is_some_and(|e| e < v.start.unwrap_or(0))
                        || v.text.as_ref().is_some_and(|t| t.len() > 1024 * 1024)
                })
            {
                return Err(ProviderError::ParseSubtitle(
                    "invalid structured subtitle".into(),
                ));
            }
            Ok(subtitle)
        } else {
            Err(ProviderError::NotFound)
        }
    }

    async fn for_you_items(&self, ctx: &Context) -> Result<Vec<ForYouEntry>, ProviderError> {
        let func = self.funcs.for_you_items.as_ref().ok_or(ProviderError::NotImplemented)?;
        let (call, _guard) = self.context(ctx, None);
        let val = func.call_async::<LuaValue>(call).await.map_err(ProviderError::from_err)?;
        self.lua.from_value(val).map_err(ProviderError::from_err)
    }

    async fn home_blocks(&self, ctx: &Context) -> Result<Vec<HomeBlock>, ProviderError> {
        let (call, _guard) = self.context(ctx, None);
        let func = self
            .funcs
            .home_blocks
            .as_ref()
            .ok_or(ProviderError::NotImplemented)?;
        let val = func
            .call_async::<LuaValue>(call.clone())
            .await
            .map_err(ProviderError::from_err)?;
        self.lua.from_value(val).map_err(ProviderError::from_err)
    }

    async fn home_block_items(
        &self,
        ctx: &Context,
        block_id: &str,
        page: i32,
        page_size: i32,
    ) -> Result<(HomeBlockContent, i32), ProviderError> {
        let (call, _guard) = self.context(ctx, None);
        let func = self
            .funcs
            .home_block_items
            .as_ref()
            .ok_or(ProviderError::NotImplemented)?;
        let val = func
            .call_async::<LuaValue>((call.clone(), block_id.to_string(), page, page_size))
            .await
            .map_err(ProviderError::from_err)?;
        let out: LuaHomeBlockItemsRsp =
            self.lua.from_value(val).map_err(ProviderError::from_err)?;
        Ok((out.content, out.total))
    }
}

#[derive(Deserialize)]
struct LuaHomeBlockItemsRsp {
    content: HomeBlockContent,
    #[serde(default)]
    total: i32,
}

#[derive(Clone, Deserialize)]
struct RadioQueueInput {
    native_id: String,
    name: String,
    description: Option<String>,
}

impl LuaUserData for LuaContext {
    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
        crate::db::add_methods(methods);
        methods.add_method("commit_album", |_, this, count: i32| {
            this.2.check()?;
            if let Some(id) = this.1 {
                let _ = this.0.ev_sender.try_send(CoreEvent::SyncCommit {
                    id,
                    commit: SyncCommit::AddAlbum(count),
                });
            }
            Ok(())
        });
        methods.add_method("commit_artist", |_, this, count: i32| {
            this.2.check()?;
            if let Some(id) = this.1 {
                let _ = this.0.ev_sender.try_send(CoreEvent::SyncCommit {
                    id,
                    commit: SyncCommit::AddArtist(count),
                });
            }
            Ok(())
        });
        methods.add_method("commit_song", |_, this, count: i32| {
            this.2.check()?;
            if let Some(id) = this.1 {
                let _ = this.0.ev_sender.try_send(CoreEvent::SyncCommit {
                    id,
                    commit: SyncCommit::AddSong(count),
                });
            }
            Ok(())
        });

        methods.add_method("get_playing", |lua, this, ()| {
            this.2.check()?;
            match this.1.and_then(qcm_core::global::get_playing) {
                Some(info) => {
                    let t = lua.create_table()?;
                    t.set("song_id", info.song_id)?;
                    t.set("native_id", info.native_id)?;
                    Ok(LuaValue::Table(t))
                }
                None => Ok(LuaValue::Nil),
            }
        });

        methods.add_async_method("sync_libraries", |lua, this, models: LuaValue| async move {
            let models: Vec<sqlm::library::Model> = lua.from_value(models)?;

            let _write = this.legacy_write().await?;
            let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
            for model in &models {
                if model.provider_id != this.provider_id()? {
                    return Err(LuaError::runtime("library belongs to another provider"));
                }
                if model.library_id != -1 {
                    this.writer()?
                        .validate_library(&txn, model.library_id)
                        .await
                        .map_err(LuaError::external)?;
                }
            }
            let conflict = [
                sqlm::library::Column::ProviderId,
                sqlm::library::Column::NativeId,
            ];

            let exclude = [sqlm::library::Column::LibraryId];

            let iter = models.into_iter().map(|i| -> sqlm::library::ActiveModel {
                let id = i.library_id;
                let mut a: sqlm::library::ActiveModel = i.into();
                if id == -1 {
                    a.library_id = NotSet
                }
                a
            });

            let out = DbOper::insert_return_key(&txn, iter, &conflict, &exclude)
                .await
                .map_err(mlua::Error::external)?;
            this.2.check()?;
            txn.commit().await.map_err(mlua::Error::external)?;

            match out {
                TryInsertResult::Inserted(out) => Ok(out),
                _ => Ok(Vec::new()),
            }
        });
        methods.add_async_method(
            "sync_albums",
            |lua, this, (models, lua_syncopt): (LuaValue, LuaValue)| async move {
                let models: Vec<sqlm::album::Model> = lua.from_value(models)?;
                let opts: Option<LuaSyncOption> = lua.from_value(lua_syncopt)?;

                let _write = this.legacy_write().await?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
                this.validate_items(
                    &txn,
                    models.iter().map(|m| m.id).collect::<Vec<_>>(),
                    &[sqlm::type_enum::ItemType::Album],
                )
                .await?;
                let conflict = [sqlm::album::Column::Id];
                let mut exclude = vec![sqlm::album::Column::Id];
                if let Some(opts) = opts {
                    exclude.extend(opts.generate_exclude::<sqlm::album::Column>());
                }
                let iter = models.into_iter().map(|i| {
                    let a: sqlm::album::ActiveModel = i.into();
                    a
                });

                let out = DbChunkOper::<50>::insert_return_key(&txn, iter, &conflict, &exclude)
                    .await
                    .map_err(mlua::Error::external)?;

                this.2.check()?;
                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(out)
            },
        );
        methods.add_async_method("sync_artists", |lua, this, models: LuaValue| async move {
            let models: Vec<sqlm::artist::Model> = lua.from_value(models)?;

            let _write = this.legacy_write().await?;
            let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
            this.validate_items(
                &txn,
                models.iter().map(|m| m.id).collect::<Vec<_>>(),
                &[sqlm::type_enum::ItemType::Artist],
            )
            .await?;
            let conflict = [sqlm::artist::Column::Id];
            let exclude = [sqlm::artist::Column::Id];
            let iter = models.into_iter().map(|i| {
                let a: sqlm::artist::ActiveModel = i.into();
                a
            });

            let out = DbChunkOper::<50>::insert_return_key(&txn, iter, &conflict, &exclude)
                .await
                .map_err(mlua::Error::external)?;

            this.2.check()?;
            txn.commit().await.map_err(mlua::Error::external)?;
            Ok(out)
        });
        methods.add_async_method("sync_songs", |lua, this, models: LuaValue| async move {
            let models: Vec<sqlm::song::Model> = lua.from_value(models)?;

            let _write = this.legacy_write().await?;
            let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
            this.validate_items(
                &txn,
                models.iter().map(|m| m.id).collect::<Vec<_>>(),
                &[sqlm::type_enum::ItemType::Song],
            )
            .await?;
            this.validate_items(
                &txn,
                models.iter().filter_map(|m| m.album_id).collect::<Vec<_>>(),
                &[sqlm::type_enum::ItemType::Album],
            )
            .await?;
            let conflict = [sqlm::song::Column::Id];
            let exclude = [sqlm::song::Column::Id];
            let iter = models.into_iter().map(|i| {
                let a: sqlm::song::ActiveModel = i.into();
                a
            });

            let out = DbChunkOper::<50>::insert_return_key(&txn, iter, &conflict, &exclude)
                .await
                .map_err(mlua::Error::external)?;

            this.2.check()?;
            txn.commit().await.map_err(mlua::Error::external)?;
            Ok(out)
        });
        methods.add_async_method(
            "sync_remote_mixes",
            |lua, this, (models, lua_syncopt): (LuaValue, LuaValue)| async move {
                let models: Vec<sqlm::remote_mix::Model> = lua.from_value(models)?;
                let opts: Option<LuaSyncOption> = lua.from_value(lua_syncopt)?;

                let _write = this.legacy_write().await?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
                this.validate_items(
                    &txn,
                    models.iter().map(|m| m.id).collect::<Vec<_>>(),
                    &[
                        sqlm::type_enum::ItemType::Mix,
                        sqlm::type_enum::ItemType::Radio,
                    ],
                )
                .await?;

                let mut out = Vec::new();
                {
                    let conflict = [sqlm::remote_mix::Column::Id];
                    let mut exclude = vec![sqlm::remote_mix::Column::Id];
                    if let Some(opts) = opts {
                        exclude.extend(opts.generate_exclude::<sqlm::remote_mix::Column>());
                    }

                    let iter = models.clone().into_iter().map(|i| {
                        let a: sqlm::remote_mix::ActiveModel = i.into();
                        a
                    });

                    out = DbChunkOper::<50>::insert_return_key(&txn, iter, &conflict, &exclude)
                        .await
                        .map_err(mlua::Error::external)?;
                }

                {
                    let now = Timestamp::now();
                    let conflict = [sqlm::mix::Column::RemoteId];
                    let exclude = [
                        sqlm::mix::Column::Id,
                        sqlm::mix::Column::SortName,
                        sqlm::mix::Column::MixType,
                        sqlm::mix::Column::CreateAt,
                        sqlm::mix::Column::ContentUpdateAt,
                    ];
                    let iter =
                        out.clone()
                            .into_iter()
                            .zip(models.into_iter())
                            .map(|(remote_id, m)| {
                                let a = sqlm::mix::ActiveModel {
                                    name: Set(m.name.clone()),
                                    remote_id: Set(Some(remote_id)),
                                    description: Set(m.description.unwrap_or_default().clone()),
                                    track_count: Set(m.track_count),
                                    create_at: Set(now),
                                    update_at: Set(now),
                                    mix_type: Set(sqlm::type_enum::MixType::Cache),
                                    id: NotSet,
                                    sort_name: NotSet,
                                    content_update_at: NotSet,
                                };
                                a
                            });

                    DbChunkOper::<50>::insert(&txn, iter, &conflict, &exclude)
                        .await
                        .map_err(mlua::Error::external)?;
                }

                this.2.check()?;
                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(out)
            },
        );
        methods.add_async_method(
            "sync_radio_queues",
            |lua, this, models: LuaValue| async move {
                let models: Vec<RadioQueueInput> = lua.from_value(models)?;
                let provider_id = this
                    .1
                    .ok_or_else(|| mlua::Error::runtime("no provider id"))?;

                let _write = this.legacy_write().await?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;

                // SQLite defaults lose milliseconds and can precede the sync cleanup cutoff.
                let now = Timestamp::now();
                let item_models = models.clone().into_iter().map(|m| sqlm::item::ActiveModel {
                    native_id: Set(m.native_id),
                    provider_id: Set(provider_id),
                    r#type: Set(sqlm::type_enum::ItemType::Radio),
                    create_at: Set(now),
                    update_at: Set(now),
                    last_sync_at: Set(now),
                    ..Default::default()
                });
                let ids = qcm_core::db::sync::allocate_items(&txn, item_models)
                    .await
                    .map_err(mlua::Error::external)?;

                let remote_mix_models: Vec<_> = ids
                    .iter()
                    .copied()
                    .zip(models.into_iter())
                    .map(|(id, m)| sqlm::remote_mix::ActiveModel {
                        id: Set(id),
                        name: Set(m.name),
                        description: Set(m.description),
                        track_count: Set(0),
                        mix_type: Set("queue".to_string()),
                    })
                    .collect();
                let conflict = [sqlm::remote_mix::Column::Id];
                let exclude = [sqlm::remote_mix::Column::Id];
                DbChunkOper::<50>::insert(&txn, remote_mix_models, &conflict, &exclude)
                    .await
                    .map_err(mlua::Error::external)?;

                this.2.check()?;
                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(ids)
            },
        );
        methods.add_async_method("sync_images", |lua, this, models: LuaValue| async move {
            let models: Vec<sqlm::image::Model> = lua.from_value(models)?;

            let _write = this.legacy_write().await?;
            let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
            this.validate_items(
                &txn,
                models.iter().map(|m| m.item_id).collect::<Vec<_>>(),
                &[],
            )
            .await?;
            let conflict = [sqlm::image::Column::ItemId, sqlm::image::Column::ImageType];
            let exclude = [sqlm::image::Column::Id];
            let iter = models.into_iter().map(|i| {
                let mut a: sqlm::image::ActiveModel = i.into();
                a.id = NotSet;
                a
            });

            DbChunkOper::<50>::insert(&txn, iter, &conflict, &exclude)
                .await
                .map_err(mlua::Error::external)?;

            this.2.check()?;
            txn.commit().await.map_err(mlua::Error::external)?;
            Ok(())
        });
        methods.add_async_method(
            "sync_dynamics",
            |lua, this, (models, lua_syncopt): (LuaValue, LuaValue)| async move {
                let models: Vec<sqlm::dynamic::Model> = lua.from_value(models)?;
                let opts: Option<LuaSyncOption> = lua.from_value(lua_syncopt)?;

                let _write = this.legacy_write().await?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
                this.validate_items(&txn, models.iter().map(|m| m.id).collect::<Vec<_>>(), &[])
                    .await?;
                let conflict = [sqlm::dynamic::Column::Id];
                let mut exclude = vec![sqlm::dynamic::Column::Id];

                if let Some(opts) = opts {
                    exclude.extend(opts.generate_exclude::<sqlm::dynamic::Column>());
                }

                let iter = models.into_iter().map(|i| {
                    let a: sqlm::dynamic::ActiveModel = i.into();
                    a
                });

                DbChunkOper::<50>::insert(&txn, iter, &conflict, &exclude)
                    .await
                    .map_err(mlua::Error::external)?;

                this.2.check()?;
                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(())
            },
        );
        methods.add_async_method(
            "sync_song_album_ids",
            |lua, this, (library_id, models): (i64, LuaValue)| async move {
                let models: Vec<(String, String)> = lua.from_value(models)?;

                let _write = this.legacy_write().await?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
                this.writer()?
                    .validate_library(&txn, library_id)
                    .await
                    .map_err(LuaError::external)?;
                sync_song_album_ids(&txn, library_id, models)
                    .await
                    .map_err(mlua::Error::external)?;

                this.2.check()?;
                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(())
            },
        );
        methods.add_async_method(
            "sync_remote_mix_song_ids",
            |_lua, this, (remote_mix_id, song_ids): (i64, Vec<i64>)| async move {
                let mix_id: i64 = sqlm::mix::Entity::find()
                    .select_only()
                    .column(sqlm::mix::Column::Id)
                    .filter(sqlm::mix::Column::RemoteId.eq(Some(remote_mix_id)))
                    .into_tuple()
                    .one(&this.0.db)
                    .await
                    .map_err(mlua::Error::external)?
                    .ok_or(mlua::Error::runtime("no mix entry"))?;

                let _write = this.legacy_write().await?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
                this.validate_items(
                    &txn,
                    [remote_mix_id],
                    &[
                        sqlm::type_enum::ItemType::Mix,
                        sqlm::type_enum::ItemType::Radio,
                    ],
                )
                .await?;
                this.validate_items(&txn, song_ids.clone(), &[sqlm::type_enum::ItemType::Song])
                    .await?;

                let conflict = [
                    sqlm::rel_mix_song::Column::MixId,
                    sqlm::rel_mix_song::Column::SongId,
                ];
                let exclude = [sqlm::rel_mix_song::Column::Id];

                let now = Timestamp::now();

                sqlm::mix::Entity::update(sqlm::mix::ActiveModel {
                    id: Set(mix_id),
                    update_at: Set(now),
                    content_update_at: Set(now),
                    ..Default::default()
                })
                .exec(&txn)
                .await
                .map_err(mlua::Error::external)?;

                let iter = song_ids.into_iter().enumerate().map(|(i, song_id)| {
                    let a: sqlm::rel_mix_song::ActiveModel = sqlm::rel_mix_song::ActiveModel {
                        mix_id: Set(mix_id),
                        song_id: Set(song_id),
                        order_idx: Set(i as i64),
                        update_at: Set(now),
                        id: NotSet,
                    };
                    a
                });

                DbChunkOper::<50>::insert(&txn, iter, &conflict, &exclude)
                    .await
                    .map_err(mlua::Error::external)?;

                this.2.check()?;
                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(())
            },
        );
        methods.add_async_method(
            "sync_album_artist_ids",
            |lua, this, (models,): (LuaValue,)| async move {
                let models: Vec<sqlm::rel_album_artist::Model> = lua.from_value(models)?;
                let _write = this.legacy_write().await?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
                this.validate_items(
                    &txn,
                    models.iter().map(|m| m.album_id).collect::<Vec<_>>(),
                    &[sqlm::type_enum::ItemType::Album],
                )
                .await?;
                this.validate_items(
                    &txn,
                    models.iter().map(|m| m.artist_id).collect::<Vec<_>>(),
                    &[sqlm::type_enum::ItemType::Artist],
                )
                .await?;

                let conflict = [
                    sqlm::rel_album_artist::Column::AlbumId,
                    sqlm::rel_album_artist::Column::ArtistId,
                ];
                let exclude = [sqlm::rel_album_artist::Column::Id];
                let iter = models.into_iter().map(|i| {
                    let mut a: sqlm::rel_album_artist::ActiveModel = i.into();
                    a.id = NotSet;
                    a
                });

                DbChunkOper::<50>::insert(&txn, iter, &conflict, &exclude)
                    .await
                    .map_err(mlua::Error::external)?;

                this.2.check()?;
                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(())
            },
        );
        methods.add_async_method(
            "sync_song_artist_ids",
            |lua, this, (models,): (LuaValue,)| async move {
                let models: Vec<sqlm::rel_song_artist::Model> = lua.from_value(models)?;
                let _write = this.legacy_write().await?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
                this.validate_items(
                    &txn,
                    models.iter().map(|m| m.song_id).collect::<Vec<_>>(),
                    &[sqlm::type_enum::ItemType::Song],
                )
                .await?;
                this.validate_items(
                    &txn,
                    models.iter().map(|m| m.artist_id).collect::<Vec<_>>(),
                    &[sqlm::type_enum::ItemType::Artist],
                )
                .await?;

                let conflict = [
                    sqlm::rel_song_artist::Column::SongId,
                    sqlm::rel_song_artist::Column::ArtistId,
                ];
                let exclude = [sqlm::rel_song_artist::Column::Id];
                let iter = models.into_iter().map(|i| {
                    let mut a: sqlm::rel_song_artist::ActiveModel = i.into();
                    a.id = NotSet;
                    a
                });

                DbChunkOper::<50>::insert(&txn, iter, &conflict, &exclude)
                    .await
                    .map_err(mlua::Error::external)?;

                this.2.check()?;
                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(())
            },
        );

        methods.add_async_method(
            "allocate_items",
            |lua, this, (models,): (LuaValue,)| async move {
                if let LuaValue::Table(t) = models.clone() {
                    if let Some(pid) = this.1 {
                        let _ = t.for_each(|_: LuaValue, v: LuaTable| {
                            if !v.contains_key("provider_id").unwrap_or(false) {
                                let _ = v.set("provider_id", pid);
                            }
                            Ok(())
                        });
                    }
                }

                let models: Vec<sqlm::item::Model> = lua.from_value(models)?;
                let _write = this.legacy_write().await?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
                for model in &models {
                    if model.provider_id != this.provider_id()? {
                        return Err(LuaError::runtime("item belongs to another provider"));
                    }
                    if let Some(id) = model.library_id {
                        this.writer()?
                            .validate_library(&txn, id)
                            .await
                            .map_err(LuaError::external)?;
                    }
                }

                let iter = models.into_iter().map(|i| {
                    let mut a: sqlm::item::ActiveModel = i.into();
                    a.id = NotSet;
                    a
                });

                let out = qcm_core::db::sync::allocate_items(&txn, iter)
                    .await
                    .map_err(mlua::Error::external)?;

                this.2.check()?;
                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(out)
            },
        );

        methods.add_async_method("libraries", |lua, this, (): ()| async move {
            this.2.check()?;
            let out = sqlm::library::Entity::find()
                .filter(sqlm::library::Column::ProviderId.eq(this.1))
                .all(&this.0.db)
                .await
                .map_err(mlua::Error::external)?;

            Ok(to_lua(&lua, &out))
        });
    }
}
