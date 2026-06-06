use crate::enums;
use crate::error::{create_lua_error_func, FromLuaError};
use crate::util::to_lua;
use mlua::prelude::*;
use qcm_core::db::sync::sync_song_album_ids;
use qcm_core::db::values::Timestamp;
use qcm_core::db::{self, DbChunkOper};
use qcm_core::event::{SyncCommit, SyncState};
use qcm_core::model as sqlm;
use qcm_core::provider::{
    AuthResult, HasCommonData, HomeBlock, HomeBlockContent, ProviderCommon, ProviderCommonData,
    QrInfo,
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
use std::str::FromStr;
use std::{
    path::Path,
    sync::Arc,
};

use crate::crypto::create_crypto_module;
use crate::http::{LuaClient, LuaResponse};
use crate::timestamp::create_time_module;
use serde::{Deserialize, Serialize};

struct LuaProviderInner {
    common: ProviderCommonData,
    client: qcm_core::http::HttpClient,
    jar: Arc<CookieStoreRwLock>,
}

struct LuaInner(Arc<LuaProviderInner>);

impl LuaUserData for LuaInner {
    fn add_fields<F: LuaUserDataFields<Self>>(fields: &mut F) {
        fields.add_field_method_get("id", |_, this| Ok(this.0.common.id()));
    }
    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
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
    get_queue_next: LuaFunction,
    favorite: LuaFunction,
    qr: Option<LuaFunction>,
    image: LuaFunction,
    audio: LuaFunction,
    subtitle: LuaFunction,
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
}

impl LuaProvider {
    pub fn new(
        id: Option<i64>,
        name: &str,
        device_id: &str,
        meta_type: &str,
        script_path: &Path,
    ) -> Result<Self> {
        let jar = Arc::new(CookieStoreRwLock::default());

        let lua = Lua::new();

        let client = qcm_core::http::client_builder_with_jar(jar.clone())
            .build()
            .unwrap();
        let inner = Arc::new(LuaProviderInner {
            common: ProviderCommonData::new(id, name, device_id, meta_type),
            client: client.clone(),
            jar: jar,
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
            qcm_table.set("crypto", create_crypto_module(&lua)?)?;
            qcm_table.set("json", create_json_module(&lua)?)?;
            qcm_table.set("time", create_time_module(&lua)?)?;
            qcm_table.set("enum", enums::create_module(&lua)?)?;
            qcm_table.set("error", create_lua_error_func(&lua)?)?;

            let inner = inner.clone();
            qcm_table.set(
                "get_http_client",
                lua.create_function(move |_, ()| {
                    Ok(LuaClient(inner.client.clone(), inner.jar.clone()))
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

        let provider = Self {
            inner,
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
                get_queue_next: provider_table
                    .get::<LuaFunction>("get_queue_next")
                    .map_err(|_| anyhow!("get_queue_next func not found"))?,
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
                subtitle: provider_table
                    .get::<LuaFunction>("subtitle")
                    .map_err(|_| anyhow!("subtitle func not found"))?,
                home_blocks: provider_table.get::<LuaFunction>("home_blocks").ok(),
                home_block_items: provider_table.get::<LuaFunction>("home_block_items").ok(),
            },
            lua,
        };
        Ok(provider)
    }

    pub fn type_name() -> &'static str {
        "lua"
    }

    pub fn client(&self) -> HttpClient {
        return self.inner.client.clone();
    }
}

impl HasCookieJar for LuaProvider {
    fn jar(&self) -> Arc<CookieStoreRwLock> {
        self.inner.jar.clone()
    }
}

impl HasCommonData for LuaProvider {
    fn common<'a>(&'a self) -> &'a ProviderCommonData {
        &self.inner.common
    }
}

#[async_trait::async_trait]
impl Provider for LuaProvider {
    fn load(&self, data: &str) {
        let _ = self.funcs.load.call::<()>(data.to_string());
    }

    fn save(&self) -> String {
        self.funcs.save.call::<String>(()).unwrap_or_default()
    }

    async fn check(&self, _ctx: &Context) -> Result<(), ProviderError> {
        self.funcs
            .check
            .call_async::<()>(())
            .await
            .map_err(ProviderError::from_err)
    }

    async fn auth(&self, _ctx: &Context, info: &AuthInfo) -> Result<AuthResult, ProviderError> {
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

        let res = self
            .funcs
            .sync
            .call_async::<LuaValue>(LuaContext(ctx.clone(), self.id()))
            .await
            .map_err(ProviderError::from_err)?;

        if res.is_integer() {
            let val = res.as_i32().and_then(|v| SyncState::try_from(v).ok());
            match val {
                Some(SyncState::NotAuth) => return Err(ProviderError::NotAuth),
                _ => {}
            }
        }

        if let Some(id) = self.id() {
            let txn = ctx.db.begin().await?;
            db::sync::sync_drop_before(&txn, id, now).await?;
            txn.commit().await?;
        }
        Ok(())
    }
    async fn sync_item(&self, ctx: &Context, item: sqlm::item::Model) -> Result<(), ProviderError> {
        self.funcs
            .sync_item
            .call_async::<LuaValue>((LuaContext(ctx.clone(), self.id()), to_lua(&self.lua, &item)))
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
        let res: LuaValue = self
            .funcs
            .get_queue_next
            .call_async((
                LuaContext(ctx.clone(), self.id()),
                native_id.to_string(),
                current_native_ids.to_vec(),
            ))
            .await
            .map_err(ProviderError::from_err)?;

        self.lua.from_value(res).map_err(ProviderError::from_err)
    }

    async fn favorite(
        &self,
        ctx: &Context,
        item_id: &str,
        item_type: sqlm::type_enum::ItemType,
        value: bool,
    ) -> Result<(), ProviderError> {
        let res = self
            .funcs
            .favorite
            .call_async::<LuaValue>((
                LuaContext(ctx.clone(), self.id()),
                item_id,
                item_type as i32,
                value,
            ))
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
            .map_err(|e| anyhow!(e))?;

        if val.is_userdata() {
            let ud = val.as_userdata().unwrap();
            let mut ud_rsp = ud.borrow_mut::<LuaResponse>().map_err(AnyError::from)?;
            let rsp = ud_rsp.0.take().ok_or(anyhow!("response not found"))?;
            Ok(rsp)
        } else {
            Err(ProviderError::NotFound)
        }
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
            .map_err(|e| anyhow!(e))?;

        if val.is_userdata() {
            let ud = val.as_userdata().unwrap();
            let mut ud_rsp = ud.borrow_mut::<LuaResponse>().map_err(AnyError::from)?;
            let rsp = ud_rsp.0.take().ok_or(anyhow!("response not found"))?;
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
            .map_err(|e| anyhow!(e))?;
        if val.is_string() {
            let lrc = val.to_string().unwrap();
            let subtitle = Subtitle::from_lrc(&lrc)
                .map_err(|e| ProviderError::ParseSubtitle(format!("{:?}", e)))?;
            Ok(subtitle)
        } else {
            Err(ProviderError::NotFound)
        }
    }

    async fn home_blocks(&self, ctx: &Context) -> Result<Vec<HomeBlock>, ProviderError> {
        let func = self
            .funcs
            .home_blocks
            .as_ref()
            .ok_or(ProviderError::NotImplemented)?;
        let val = func
            .call_async::<LuaValue>(LuaContext(ctx.clone(), self.id()))
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
        let func = self
            .funcs
            .home_block_items
            .as_ref()
            .ok_or(ProviderError::NotImplemented)?;
        let val = func
            .call_async::<LuaValue>((
                LuaContext(ctx.clone(), self.id()),
                block_id.to_string(),
                page,
                page_size,
            ))
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

fn create_json_module(lua: &Lua) -> LuaResult<LuaTable> {
    let t = lua.create_table()?;
    t.set(
        "encode",
        lua.create_function(|_, v: mlua::Value| {
            serde_json::to_string(&v).map_err(|e| mlua::Error::external(e))
        })?,
    )?;
    t.set(
        "decode",
        lua.create_function(|lua, str: String| {
            let v: serde_json::Value =
                serde_json::from_str(&str).map_err(|e| mlua::Error::external(e))?;
            to_lua(&lua, &v)
        })?,
    )?;
    Ok(t)
}

#[derive(Clone, Deserialize)]
struct RadioQueueInput {
    native_id: String,
    name: String,
    description: Option<String>,
}

struct LuaContext(Context, /* provider_id */ Option<i64>);

impl LuaUserData for LuaContext {
    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
        methods.add_method("commit_album", |_, this, count: i32| {
            if let Some(id) = this.1 {
                let _ = this.0.ev_sender.try_send(CoreEvent::SyncCommit {
                    id,
                    commit: SyncCommit::AddAlbum(count),
                });
            }
            Ok(())
        });
        methods.add_method("commit_artist", |_, this, count: i32| {
            if let Some(id) = this.1 {
                let _ = this.0.ev_sender.try_send(CoreEvent::SyncCommit {
                    id,
                    commit: SyncCommit::AddArtist(count),
                });
            }
            Ok(())
        });
        methods.add_method("commit_song", |_, this, count: i32| {
            if let Some(id) = this.1 {
                let _ = this.0.ev_sender.try_send(CoreEvent::SyncCommit {
                    id,
                    commit: SyncCommit::AddSong(count),
                });
            }
            Ok(())
        });

        methods.add_method("get_playing", |lua, this, ()| {
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

            let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
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

                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
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

                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(out)
            },
        );
        methods.add_async_method("sync_artists", |lua, this, models: LuaValue| async move {
            let models: Vec<sqlm::artist::Model> = lua.from_value(models)?;

            let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
            let conflict = [sqlm::artist::Column::Id];
            let exclude = [sqlm::artist::Column::Id];
            let iter = models.into_iter().map(|i| {
                let a: sqlm::artist::ActiveModel = i.into();
                a
            });

            let out = DbChunkOper::<50>::insert_return_key(&txn, iter, &conflict, &exclude)
                .await
                .map_err(mlua::Error::external)?;

            txn.commit().await.map_err(mlua::Error::external)?;
            Ok(out)
        });
        methods.add_async_method("sync_songs", |lua, this, models: LuaValue| async move {
            let models: Vec<sqlm::song::Model> = lua.from_value(models)?;

            let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
            let conflict = [sqlm::song::Column::Id];
            let exclude = [sqlm::song::Column::Id];
            let iter = models.into_iter().map(|i| {
                let a: sqlm::song::ActiveModel = i.into();
                a
            });

            let out = DbChunkOper::<50>::insert_return_key(&txn, iter, &conflict, &exclude)
                .await
                .map_err(mlua::Error::external)?;

            txn.commit().await.map_err(mlua::Error::external)?;
            Ok(out)
        });
        methods.add_async_method(
            "sync_remote_mixes",
            |lua, this, (models, lua_syncopt): (LuaValue, LuaValue)| async move {
                let models: Vec<sqlm::remote_mix::Model> = lua.from_value(models)?;
                let opts: Option<LuaSyncOption> = lua.from_value(lua_syncopt)?;

                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;

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

                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(out)
            },
        );
        methods.add_async_method(
            "sync_radio_queues",
            |lua, this, models: LuaValue| async move {
                let models: Vec<RadioQueueInput> = lua.from_value(models)?;
                let provider_id = this.1.ok_or_else(|| mlua::Error::runtime("no provider id"))?;

                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;

                let item_models = models.clone().into_iter().map(|m| sqlm::item::ActiveModel {
                    native_id: Set(m.native_id),
                    provider_id: Set(provider_id),
                    r#type: Set(sqlm::type_enum::ItemType::Radio),
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

                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(ids)
            },
        );
        methods.add_async_method("sync_images", |lua, this, models: LuaValue| async move {
            let models: Vec<sqlm::image::Model> = lua.from_value(models)?;

            let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
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

            txn.commit().await.map_err(mlua::Error::external)?;
            Ok(())
        });
        methods.add_async_method(
            "sync_dynamics",
            |lua, this, (models, lua_syncopt): (LuaValue, LuaValue)| async move {
                let models: Vec<sqlm::dynamic::Model> = lua.from_value(models)?;
                let opts: Option<LuaSyncOption> = lua.from_value(lua_syncopt)?;

                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
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

                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(())
            },
        );
        methods.add_async_method(
            "sync_song_album_ids",
            |lua, this, (library_id, models): (i64, LuaValue)| async move {
                let models: Vec<(String, String)> = lua.from_value(models)?;

                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;
                sync_song_album_ids(&txn, library_id, models)
                    .await
                    .map_err(mlua::Error::external)?;

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

                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;

                let conflict = [
                    sqlm::rel_mix_song::Column::MixId,
                    sqlm::rel_mix_song::Column::SongId,
                ];
                let exclude = [sqlm::rel_mix_song::Column::Id];

                let now = Timestamp::now();

                sqlm::mix::Entity::update(sqlm::mix::ActiveModel {
                    id: Set(mix_id),
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

                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(())
            },
        );
        methods.add_async_method(
            "sync_album_artist_ids",
            |lua, this, (models,): (LuaValue,)| async move {
                let models: Vec<sqlm::rel_album_artist::Model> = lua.from_value(models)?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;

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

                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(())
            },
        );
        methods.add_async_method(
            "sync_song_artist_ids",
            |lua, this, (models,): (LuaValue,)| async move {
                let models: Vec<sqlm::rel_song_artist::Model> = lua.from_value(models)?;
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;

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
                let txn = this.0.db.begin().await.map_err(mlua::Error::external)?;

                let iter = models.into_iter().map(|i| {
                    let mut a: sqlm::item::ActiveModel = i.into();
                    a.id = NotSet;
                    a
                });

                let out = qcm_core::db::sync::allocate_items(&txn, iter)
                    .await
                    .map_err(mlua::Error::external)?;

                txn.commit().await.map_err(mlua::Error::external)?;
                Ok(out)
            },
        );

        methods.add_async_method("libraries", |lua, this, (): ()| async move {
            let out = sqlm::library::Entity::find()
                .filter(sqlm::library::Column::ProviderId.eq(this.1))
                .all(&this.0.db)
                .await
                .map_err(mlua::Error::external)?;

            Ok(to_lua(&lua, &out))
        });
    }
}
