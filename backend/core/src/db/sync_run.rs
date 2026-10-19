use super::sync_batch::{ItemRef, ProviderWriter, SyncBatch, SyncResult};
use crate::model::{self as m, type_enum::ItemType};
use sea_orm::*;
use serde::{Deserialize, Serialize};
use std::{
    collections::{HashMap, HashSet},
    sync::{
        atomic::{AtomicU64, Ordering},
        Arc, Mutex,
    },
};
use tokio::sync::{Mutex as AsyncMutex, OwnedMutexGuard};

fn error(message: &str) -> DbErr {
    DbErr::Custom(message.into())
}
const MAX_SEEN: usize = 200_000;
#[derive(Debug, Default)]
pub(super) struct WriteState {
    revision: u64,
    changes: u64,
    favorites: HashMap<i64, u64>,
    favorite_overflow: u64,
}
#[derive(Debug, Default)]
pub(super) struct ProviderState {
    pub(super) writes: Arc<AsyncMutex<WriteState>>,
    pub(super) retained: Arc<super::retention::RetainedItems>,
    active: AtomicU64,
    sequence: AtomicU64,
}
#[derive(Debug, Default)]
pub struct SyncCoordinator {
    providers: Mutex<HashMap<i64, Arc<ProviderState>>>,
}
impl SyncCoordinator {
    pub(super) fn state(&self, provider: i64) -> Arc<ProviderState> {
        self.providers
            .lock()
            .unwrap()
            .entry(provider)
            .or_default()
            .clone()
    }
    pub async fn read_revision(&self, provider: i64) -> u64 {
        self.state(provider).writes.lock().await.changes
    }
    pub async fn write(&self, provider: i64, origin: Option<u64>) -> Result<WriteGuard, DbErr> {
        let state = self.state(provider);
        let mut guard = state.writes.clone().lock_owned().await;
        if let Some(token) = origin {
            if state.active.load(Ordering::Acquire) != token {
                return Err(error("sync session is no longer active"));
            }
        } else {
            guard.revision = guard
                .revision
                .checked_add(1)
                .ok_or_else(|| error("write revision exhausted"))?;
        }
        guard.changes = guard.changes.checked_add(1).ok_or_else(|| error("read revision exhausted"))?;
        Ok(WriteGuard { guard, state })
    }
    pub async fn write_items(
        &self,
        db: &DatabaseConnection,
        ids: &[i64],
    ) -> Result<Vec<WriteGuard>, DbErr> {
        let mut providers: Vec<i64> = m::item::Entity::find()
            .select_only()
            .column(m::item::Column::ProviderId)
            .filter(m::item::Column::Id.is_in(ids.iter().copied()))
            .distinct()
            .into_tuple()
            .all(db)
            .await?;
        providers.sort_unstable();
        let mut guards = Vec::new();
        for provider in providers {
            guards.push(self.write(provider, None).await?);
        }
        Ok(guards)
    }
    pub async fn write_mixes(
        &self,
        db: &DatabaseConnection,
        ids: &[i64],
        items: &[i64],
    ) -> Result<Vec<WriteGuard>, DbErr> {
        let remotes: Vec<Option<i64>> = m::mix::Entity::find()
            .select_only()
            .column(m::mix::Column::RemoteId)
            .filter(m::mix::Column::Id.is_in(ids.iter().copied()))
            .into_tuple()
            .all(db)
            .await?;
        let mut affected = items.to_vec();
        affected.extend(remotes.into_iter().flatten());
        self.write_items(db, &affected).await
    }
    pub async fn start(&self, provider: i64) -> Result<SyncRun, DbErr> {
        let state = self.state(provider);
        let mut guard = state.writes.clone().lock_owned().await;
        if state.active.load(Ordering::Acquire) != 0 {
            return Err(error("full sync is already running"));
        }
        let token = state
            .sequence
            .fetch_add(1, Ordering::Relaxed)
            .checked_add(1)
            .ok_or_else(|| error("sync generation exhausted"))?;
        state.active.store(token, Ordering::Release);
        guard.favorites.clear();
        guard.favorite_overflow = 0;
        Ok(SyncRun {
            state: state.clone(),
            provider,
            token,
            revision: guard.revision,
            scopes: HashSet::new(),
            complete: HashSet::new(),
            seen: HashSet::new(),
            libraries: HashSet::new(),
            libraries_complete: false,
            declared: false,
            sealed: false,
            failed: false,
            generation: None,
        })
    }
}
pub struct WriteGuard {
    guard: OwnedMutexGuard<WriteState>,
    state: Arc<ProviderState>,
}
impl WriteGuard {
    pub fn retained_items(&self) -> HashSet<i64> {
        self.state.retained.0.lock().unwrap().keys().copied().collect()
    }
    pub fn invalidate_sync(&self) {
        self.state.active.store(0, Ordering::Release);
    }
    pub fn check_origin(&self, token: Option<u64>) -> Result<(), DbErr> {
        if token.is_some_and(|token| self.state.active.load(Ordering::Acquire) != token) {
            Err(error("sync session is no longer active"))
        } else {
            Ok(())
        }
    }
    pub fn favorite(&mut self, id: i64) {
        if self.state.active.load(Ordering::Acquire) != 0 {
            let revision = self.guard.revision;
            if self.guard.favorites.len() < MAX_SEEN {
                self.guard.favorites.insert(id, revision);
            } else {
                self.guard.favorite_overflow = revision;
            }
        }
    }
    pub fn favorite_changed_after(&self, id: i64, revision: u64) -> bool {
        self.guard.favorite_overflow > revision
            || self.guard.favorites.get(&id).is_some_and(|r| *r > revision)
    }
}

#[derive(Clone, Debug, Deserialize, Serialize, PartialEq, Eq, Hash)]
#[serde(deny_unknown_fields)]
pub struct SyncScope {
    pub library_id: Option<i64>,
    pub item_type: i32,
}
impl SyncScope {
    pub fn new(library: Option<i64>, item_type: ItemType) -> Self {
        Self {
            library_id: library,
            item_type: item_type as i32,
        }
    }
}
#[derive(Clone)]
pub struct SyncCancellation {
    state: Arc<ProviderState>,
    token: u64,
}
impl SyncCancellation {
    pub fn cancel(&self) {
        let _ =
            self.state
                .active
                .compare_exchange(self.token, 0, Ordering::AcqRel, Ordering::Acquire);
    }
}
#[derive(Clone)]
pub struct WriteGeneration {
    pub current: Arc<AtomicU64>,
    pub expected: u64,
}
impl WriteGeneration {
    pub fn check(&self) -> Result<(), DbErr> {
        if self.current.load(Ordering::Acquire) != self.expected {
            Err(error("provider generation changed"))
        } else {
            Ok(())
        }
    }
}
pub struct SyncRun {
    state: Arc<ProviderState>,
    provider: i64,
    token: u64,
    revision: u64,
    scopes: HashSet<SyncScope>,
    complete: HashSet<SyncScope>,
    seen: HashSet<i64>,
    libraries: HashSet<i64>,
    libraries_complete: bool,
    declared: bool,
    sealed: bool,
    failed: bool,
    generation: Option<WriteGeneration>,
}
impl Drop for SyncRun {
    fn drop(&mut self) {
        let _ =
            self.state
                .active
                .compare_exchange(self.token, 0, Ordering::AcqRel, Ordering::Acquire);
    }
}
impl SyncRun {
    pub fn cancellation(&self) -> SyncCancellation {
        SyncCancellation {
            state: self.state.clone(),
            token: self.token,
        }
    }
    pub fn bind_generation(&mut self, generation: WriteGeneration) {
        self.generation = Some(generation);
    }
    pub fn token(&self) -> u64 {
        self.token
    }
    pub fn revision(&self) -> u64 {
        self.revision
    }
    fn running(&self) -> Result<(), DbErr> {
        if let Some(generation) = &self.generation {
            generation.check()?;
        }
        if self.failed || self.sealed || self.state.active.load(Ordering::Acquire) != self.token {
            return Err(error("sync session is not writable"));
        }
        Ok(())
    }
    pub fn fail(&mut self) {
        self.failed = true;
    }
    pub async fn observe_libraries(
        &mut self,
        db: &DatabaseConnection,
        ids: Vec<i64>,
    ) -> Result<(), DbErr> {
        self.running()?;
        if self.declared {
            return Err(error(
                "libraries must be observed before scopes are declared",
            ));
        }
        if ids.len() + self.libraries.len() > MAX_SEEN {
            self.failed = true;
            return Err(error("observation limit exceeded"));
        }
        for id in ids {
            let library = m::library::Entity::find_by_id(id)
                .one(db)
                .await?
                .ok_or_else(|| error("missing observed library"))?;
            if library.provider_id != self.provider {
                self.failed = true;
                return Err(error("observed library belongs to another provider"));
            }
            self.libraries.insert(id);
        }
        Ok(())
    }
    pub fn declare(
        &mut self,
        scopes: Vec<SyncScope>,
        required_types: &[ItemType],
    ) -> Result<(), DbErr> {
        self.running()?;
        if self.declared {
            return Err(error("scopes have already been declared"));
        }
        let selected: HashSet<_> = scopes.iter().cloned().collect();
        if selected.len() != scopes.len() || scopes.len() > MAX_SEEN {
            return Err(error("duplicate or excessive scopes"));
        }
        for scope in &selected {
            let kind =
                ItemType::try_from(scope.item_type).map_err(|_| error("invalid scope type"))?;
            if let Some(id) = scope.library_id {
                if !self.libraries.contains(&id) || !required_types.contains(&kind) {
                    return Err(error(
                        "scope is outside discovered libraries or required types",
                    ));
                }
            } else if !matches!(kind, ItemType::Mix | ItemType::Radio) {
                return Err(error("only remote mixes and radios have a global full-sync scope"));
            }
        }
        for id in &self.libraries {
            for kind in required_types {
                if !selected.contains(&SyncScope::new(Some(*id), *kind)) {
                    return Err(error("required library scope is missing"));
                }
            }
        }
        if !selected.contains(&SyncScope::new(None, ItemType::Mix)) {
            return Err(error("global mix scope is missing"));
        }
        self.scopes = selected;
        self.declared = true;
        Ok(())
    }
    pub fn complete_libraries(&mut self) -> Result<(), DbErr> {
        self.running()?;
        self.libraries_complete = true;
        Ok(())
    }
    pub fn complete(&mut self, scope: SyncScope) -> Result<(), DbErr> {
        self.running()?;
        if !self.scopes.contains(&scope) {
            return Err(error("unknown sync scope"));
        }
        self.complete.insert(scope);
        Ok(())
    }
    pub async fn observe_items(
        &mut self,
        db: &DatabaseConnection,
        coordinator: &SyncCoordinator,
        scope: &SyncScope,
        items: &[ItemRef],
    ) -> Result<(), DbErr> {
        self.running()?;
        if !self.scopes.contains(scope) || self.complete.contains(scope) {
            return Err(error("scope is not writable"));
        }
        if items.len() + self.seen.len() > MAX_SEEN {
            self.failed = true;
            return Err(error("observation limit exceeded"));
        }
        let _guard = coordinator.write(self.provider, Some(self.token)).await?;
        let txn = db.begin().await?;
        let writer = ProviderWriter::new(db, self.provider, coordinator);
        let mut ids = Vec::new();
        for key in items {
            let item = writer.resolve(&txn, key, &[]).await?;
            if item.library_id != scope.library_id || item.r#type as i32 != scope.item_type {
                self.failed = true;
                return Err(error("observed item is outside scope"));
            }
            ids.push(item.id);
        }
        txn.commit().await?;
        self.seen.extend(ids);
        Ok(())
    }
    pub async fn sync_batch(
        &mut self,
        db: &DatabaseConnection,
        coordinator: &SyncCoordinator,
        batch: SyncBatch,
    ) -> Result<SyncResult, DbErr> {
        self.running()?;
        let result = ProviderWriter::new(db, self.provider, coordinator)
            .from_run(self.token, self.revision)
            .with_generation(self.generation.clone())
            .sync_batch(batch)
            .await;
        if result.is_err() {
            self.failed = true;
        }
        result
    }
    pub fn seal(&mut self) -> Result<(), DbErr> {
        self.running()?;
        if !self.declared || !self.libraries_complete || self.complete != self.scopes {
            return Err(error("sync enumeration is incomplete"));
        }
        self.sealed = true;
        Ok(())
    }
    pub async fn finalize(
        &mut self,
        db: &DatabaseConnection,
        coordinator: &SyncCoordinator,
    ) -> Result<(), DbErr> {
        if self.failed || !self.sealed {
            return Err(error("sync was not sealed successfully"));
        }
        let guard = coordinator.write(self.provider, Some(self.token)).await?;
        if guard.guard.revision != self.revision {
            self.failed = true;
            return Err(error(
                "sync prune conflicted with another write; retry required",
            ));
        }
        let txn = db.begin().await?;
        // Candidate enumeration is bounded and deletions share the write guard and transaction.
        let candidates = m::item::Entity::find()
            .filter(m::item::Column::ProviderId.eq(self.provider))
            .limit((MAX_SEEN + 1) as u64)
            .all(&txn)
            .await?;
        if candidates.len() > MAX_SEEN {
            self.failed = true;
            return Err(error("prune candidate limit exceeded"));
        }
        let retained = guard.retained_items();
        for item in candidates {
            let scope = SyncScope::new(item.library_id, item.r#type);
            if self.scopes.contains(&scope)
                && !self.seen.contains(&item.id)
                && !retained.contains(&item.id)
            {
                if matches!(item.r#type, ItemType::Mix | ItemType::Radio) {
                    m::mix::Entity::delete_many()
                        .filter(m::mix::Column::RemoteId.eq(item.id))
                        .exec(&txn)
                        .await?;
                }
                m::item::Entity::delete_by_id(item.id).exec(&txn).await?;
            }
        }
        let libraries = m::library::Entity::find()
            .filter(m::library::Column::ProviderId.eq(self.provider))
            .limit((MAX_SEEN + 1) as u64)
            .all(&txn)
            .await?;
        if libraries.len() > MAX_SEEN {
            return Err(error("library limit exceeded"));
        }
        for library in libraries {
            if !self.libraries.contains(&library.library_id) {
                library.delete(&txn).await?;
            }
        }
        if let Some(generation) = &self.generation {
            generation.check()?;
        }
        guard.check_origin(Some(self.token))?;
        txn.commit().await?;
        self.state.active.store(0, Ordering::Release);
        Ok(())
    }
}
