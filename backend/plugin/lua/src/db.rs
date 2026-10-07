use mlua::prelude::*;
use qcm_core::{
    db::{
        sync_batch::{ItemRef, ProviderWriter, SyncBatch},
        sync_run::{SyncRun, SyncScope},
    },
    model::type_enum::ItemType,
    provider::Context,
};
use std::sync::{
    atomic::{AtomicBool, AtomicU64, Ordering},
    Arc,
};
use tokio::sync::Mutex;

pub struct CallState {
    active: AtomicBool,
    generation: Arc<AtomicU64>,
    started: u64,
    pub explicit: bool,
    pub run: Option<Arc<Mutex<SyncRun>>>,
    pub required_types: Vec<ItemType>,
    cancellation: Option<qcm_core::db::sync_run::SyncCancellation>,
}
pub struct CallGuard(Arc<CallState>);
impl Drop for CallGuard {
    fn drop(&mut self) {
        self.0.active.store(false, Ordering::Release);
        if let Some(cancel) = &self.0.cancellation {
            cancel.cancel();
        }
    }
}
impl CallState {
    pub fn new(
        generation: Arc<AtomicU64>,
        explicit: bool,
        mut run: Option<SyncRun>,
        required_types: Vec<ItemType>,
    ) -> (Arc<Self>, CallGuard) {
        let started = generation.load(Ordering::Acquire);
        if let Some(run) = &mut run {
            run.bind_generation(qcm_core::db::sync_run::WriteGeneration {
                current: generation.clone(),
                expected: started,
            });
        }
        let cancellation = run.as_ref().map(SyncRun::cancellation);
        let state = Arc::new(Self {
            cancellation,
            active: AtomicBool::new(true),
            started,
            generation,
            explicit,
            run: run.map(|r| Arc::new(Mutex::new(r))),
            required_types,
        });
        (state.clone(), CallGuard(state))
    }
    pub fn write_generation(&self) -> qcm_core::db::sync_run::WriteGeneration {
        qcm_core::db::sync_run::WriteGeneration {
            current: self.generation.clone(),
            expected: self.started,
        }
    }
    pub fn check(&self) -> LuaResult<()> {
        if !self.active.load(Ordering::Acquire)
            || self.generation.load(Ordering::Acquire) != self.started
        {
            return Err(LuaError::runtime("provider context is no longer active"));
        }
        Ok(())
    }
}
#[derive(Clone)]
pub struct LuaContext(pub Context, pub Option<i64>, pub Arc<CallState>);
impl LuaContext {
    async fn decode<T: serde::de::DeserializeOwned + Send>(
        &self,
        lua: &Lua,
        value: LuaValue,
    ) -> LuaResult<T> {
        let result = lua.from_value(value);
        if result.is_err() {
            if let Some(run) = &self.2.run {
                run.lock().await.fail();
            }
        }
        result
    }
    pub fn writer(&self) -> LuaResult<ProviderWriter<'_>> {
        Ok(ProviderWriter::new(
            &self.0.db,
            self.provider_id()?,
            &self.0.sync_coordinator,
        ))
    }
    pub async fn validate_items(
        &self,
        txn: &sea_orm::DatabaseTransaction,
        ids: impl IntoIterator<Item = i64>,
        kinds: &[ItemType],
    ) -> LuaResult<()> {
        let writer = self.writer()?;
        for id in ids {
            writer
                .resolve(txn, &ItemRef::Id(id), kinds)
                .await
                .map_err(LuaError::external)?;
        }
        Ok(())
    }
    fn publish(&self, result: &qcm_core::db::sync_batch::SyncResult) -> LuaResult<()> {
        let id = self.provider_id()?;
        for commit in [
            qcm_core::event::SyncCommit::AddAlbum(result.albums.len() as i32),
            qcm_core::event::SyncCommit::AddArtist(result.artists.len() as i32),
            qcm_core::event::SyncCommit::AddSong(result.songs.len() as i32),
        ] {
            let _ = self
                .0
                .ev_sender
                .try_send(qcm_core::event::Event::SyncCommit { id, commit });
        }
        Ok(())
    }
    pub fn provider_id(&self) -> LuaResult<i64> {
        self.2.check()?;
        self.1
            .ok_or_else(|| LuaError::runtime("provider has no persisted id"))
    }
    pub async fn legacy_write(&self) -> LuaResult<qcm_core::db::sync_run::WriteGuard> {
        if self.2.explicit {
            return Err(LuaError::runtime("explicit plugins must use sync_batch"));
        }
        let id = self.provider_id()?;
        self.0
            .sync_coordinator
            .write(id, None)
            .await
            .map_err(LuaError::external)
    }
}

#[derive(serde::Deserialize)]
#[serde(deny_unknown_fields)]
struct BeginSync {
    libraries: Vec<i64>,
    scopes: Vec<SyncScope>,
}
struct LuaRun(LuaContext);

pub fn add_methods<M: LuaUserDataMethods<LuaContext>>(methods: &mut M) {
    methods.add_async_function(
        "resolve_items",
        |lua, (ud, kind, natives): (LuaAnyUserData, String, Vec<String>)| async move {
            let ctx = ud.borrow::<LuaContext>()?.clone();
            let id = ctx.provider_id()?;
            if natives.len() > 500 {
                return Err(LuaError::runtime("reference limit exceeded"));
            }
            let kind: ItemType = kind.parse().map_err(LuaError::external)?;
            let writer = ProviderWriter::new(&ctx.0.db, id, &ctx.0.sync_coordinator);
            let mut items = Vec::new();
            for native in natives {
                items.push(
                    writer
                        .resolve_unique(&native, kind)
                        .await
                        .map_err(LuaError::external)?,
                );
            }
            lua.to_value(&items)
        },
    );
    methods.add_async_function(
        "sync_batch",
        |lua, (ud, value): (LuaAnyUserData, LuaValue)| async move {
            let ctx = ud.borrow::<LuaContext>()?.clone();
            let id = ctx.provider_id()?;
            let batch: SyncBatch = ctx.decode(&lua, value).await?;
            let result = if let Some(run) = &ctx.2.run {
                run.lock()
                    .await
                    .sync_batch(&ctx.0.db, &ctx.0.sync_coordinator, batch)
                    .await
            } else {
                ProviderWriter::new(&ctx.0.db, id, &ctx.0.sync_coordinator)
                    .with_generation(Some(ctx.2.write_generation()))
                    .sync_batch(batch)
                    .await
            }
            .map_err(LuaError::external)?;
            ctx.2.check()?;
            ctx.publish(&result)?;
            lua.to_value(&result)
        },
    );
    methods.add_async_function(
        "begin_sync",
        |lua, (ud, value): (LuaAnyUserData, LuaValue)| async move {
            let ctx = ud.borrow::<LuaContext>()?.clone();
            ctx.provider_id()?;
            let options: BeginSync = ctx.decode(&lua, value).await?;
            let run = ctx.2.run.as_ref().ok_or_else(|| {
                LuaError::runtime("begin_sync requires an explicit full-sync call")
            })?;
            let mut run = run.lock().await;
            let result = async {
                run.observe_libraries(&ctx.0.db, options.libraries).await?;
                run.declare(options.scopes, &ctx.2.required_types)
            }
            .await;
            if result.is_err() {
                run.fail();
            }
            result.map_err(LuaError::external)?;
            drop(run);
            Ok(LuaRun(ctx))
        },
    );
}
impl LuaUserData for LuaRun {
    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
        methods.add_async_function(
            "sync_batch",
            |lua, (ud, value): (LuaAnyUserData, LuaValue)| async move {
                let ctx = ud.borrow::<LuaRun>()?.0.clone();
                ctx.provider_id()?;
                let batch = ctx.decode(&lua, value).await?;
                let mut run = ctx.2.run.as_ref().unwrap().lock().await;
                let result = run
                    .sync_batch(&ctx.0.db, &ctx.0.sync_coordinator, batch)
                    .await
                    .map_err(LuaError::external)?;
                ctx.publish(&result)?;
                lua.to_value(&result)
            },
        );
        methods.add_async_function(
            "observe_libraries",
            |lua, (ud, ids): (LuaAnyUserData, LuaValue)| async move {
                let ctx = ud.borrow::<LuaRun>()?.0.clone();
                ctx.provider_id()?;
                let ids: Vec<i64> = ctx.decode(&lua, ids).await?;
                let mut run = ctx.2.run.as_ref().unwrap().lock().await;
                let result = run.observe_libraries(&ctx.0.db, ids).await;
                if result.is_err() {
                    run.fail();
                }
                result.map_err(LuaError::external)
            },
        );
        methods.add_async_function(
            "observe_items",
            |lua, (ud, scope, items): (LuaAnyUserData, LuaValue, LuaValue)| async move {
                let ctx = ud.borrow::<LuaRun>()?.0.clone();
                ctx.provider_id()?;
                let scope: SyncScope = ctx.decode(&lua, scope).await?;
                let items: Vec<ItemRef> = ctx.decode(&lua, items).await?;
                let mut run = ctx.2.run.as_ref().unwrap().lock().await;
                let result = run
                    .observe_items(&ctx.0.db, &ctx.0.sync_coordinator, &scope, &items)
                    .await;
                if result.is_err() {
                    run.fail();
                }
                result.map_err(LuaError::external)
            },
        );
        methods.add_async_function("complete",|lua,(ud,scope):(LuaAnyUserData,LuaValue)| async move {
            let ctx=ud.borrow::<LuaRun>()?.0.clone();ctx.provider_id()?;
            let scope=if matches!(&scope,LuaValue::String(s) if s.as_bytes().as_ref()==b"libraries") {None} else {Some(ctx.decode::<SyncScope>(&lua,scope).await?)};
            let mut run=ctx.2.run.as_ref().unwrap().lock().await;
            let result=match scope {Some(scope)=>run.complete(scope),None=>run.complete_libraries()};
            if result.is_err(){run.fail();}
            result.map_err(LuaError::external)
        });
        methods.add_async_function("seal", |_, ud: LuaAnyUserData| async move {
            let ctx = ud.borrow::<LuaRun>()?.0.clone();
            ctx.provider_id()?;
            let mut run = ctx.2.run.as_ref().unwrap().lock().await;
            let result = run.seal();
            if result.is_err() {
                run.fail();
            }
            result.map_err(LuaError::external)
        });
    }
}
