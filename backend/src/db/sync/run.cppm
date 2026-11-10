module;
#include <rstd/macro.hpp>
export module qcm.backend:db.sync.run;
export import rstd;
import :uuid;
import sqlito;
using rstd::tuple;
using rstd::collections::HashMap;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using rstd::sync::MutexGuard;
using rstd::sync::RwLock;
using rstd::sync::RwLockReadGuard;
using rstd::sync::RwLockWriteGuard;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct ProjectionSnapshot {
    String               instance;
    u64                  revision {};
    Vec<tuple<i64, u64>> epochs;
};
class SyncGuard {
public:
    virtual ~SyncGuard()                                              = default;
    virtual auto check_origin() const -> Result<empty, sqlito::Error> = 0;
    virtual auto favorite_changed_after(i64 item) const -> bool       = 0;
};
struct ProviderWriteState {
    u64                 active {};
    u64                 sequence {};
    u64                 revision {};
    u64                 favorite_overflow {};
    HashMap<i64, u64>   favorites;
    HashMap<i64, usize> retained;
};
using ProviderStateHandle = Arc<Mutex<ProviderWriteState>>;
struct CoordinatorMetadata {
    String               instance;
    u64                  revision { 1 };
    Vec<tuple<i64, u64>> epochs;
};
struct CoordinatorState {
    HashMap<i64, ProviderStateHandle> providers;
};
using ProjectionState = Arc<RwLock<empty>>;
class ProjectionWriteLease {
    ProjectionState        owner_;
    RwLockReadGuard<empty> lock_;

public:
    explicit ProjectionWriteLease(ProjectionState owner)
        : owner_(rstd::move(owner)), lock_(owner_->read().unwrap()) {}
    ProjectionWriteLease(const ProjectionWriteLease&)                    = delete;
    auto operator=(const ProjectionWriteLease&) -> ProjectionWriteLease& = delete;
    ProjectionWriteLease(ProjectionWriteLease&&)                         = delete;
    auto operator=(ProjectionWriteLease&&) -> ProjectionWriteLease&      = delete;
};
using ProjectionWriteLock = Arc<ProjectionWriteLease>;
struct ProjectionRead {
    ProjectionState         owner;
    RwLockWriteGuard<empty> lock;
    ProjectionSnapshot      snapshot;

    ProjectionRead(ProjectionState owner, RwLockWriteGuard<empty> lock, ProjectionSnapshot snapshot)
        : owner(rstd::move(owner)), lock(rstd::move(lock)), snapshot(rstd::move(snapshot)) {}
    ProjectionRead(const ProjectionRead&)                    = delete;
    auto operator=(const ProjectionRead&) -> ProjectionRead& = delete;
    ProjectionRead(ProjectionRead&&) noexcept                = default;
    auto operator=(ProjectionRead&&) -> ProjectionRead&      = delete;
};
inline auto sync_guard_error(ref<str> message) -> sqlito::Error {
    return sqlito::Error::from(sqlito::ErrorKind::Input, 0, String::make(message));
}
class ItemRetention {
    ProviderStateHandle state_;
    Vec<i64>            roots_;
    Vec<i64>            ids_;

public:
    ItemRetention(ProviderStateHandle state, Vec<i64> roots, Vec<i64> ids)
        : state_(rstd::move(state)), roots_(rstd::move(roots)), ids_(rstd::move(ids)) {}
    ItemRetention(const ItemRetention&)                    = delete;
    auto operator=(const ItemRetention&) -> ItemRetention& = delete;
    ~ItemRetention() {
        auto state = state_->lock().unwrap();
        for (auto id : ids_.as_slice()) {
            auto found = state->retained.get_mut(id);
            if (found.is_some() && --**found == usize()) (void)state->retained.remove(id);
        }
    }
    auto roots() const -> slice<i64> { return roots_.as_slice(); }
};
using ItemRetentionHandle = Arc<ItemRetention>;
class ProviderWriteGuard final : public SyncGuard {
    ProviderStateHandle            state_;
    Option<ProjectionWriteLock>    projection_;
    MutexGuard<ProviderWriteState> lock_;
    u64                            token_ {};
    u64                            revision_ {};
    bool                           finalize_ {};

public:
    ProviderWriteGuard(ProviderStateHandle state, Option<ProjectionWriteLock> projection,
                       MutexGuard<ProviderWriteState> lock, u64 token, u64 revision, bool finalize)
        : state_(rstd::move(state)),
          projection_(rstd::move(projection)),
          lock_(rstd::move(lock)),
          token_(token),
          revision_(revision),
          finalize_(finalize) {}
    ProviderWriteGuard(const ProviderWriteGuard&)                    = delete;
    auto operator=(const ProviderWriteGuard&) -> ProviderWriteGuard& = delete;
    ProviderWriteGuard(ProviderWriteGuard&&)                         = delete;
    auto operator=(ProviderWriteGuard&&) -> ProviderWriteGuard&      = delete;
    auto check_origin() const -> Result<empty, sqlito::Error> override {
        if ((token_ != u64() && lock_->active != token_) ||
            (finalize_ && lock_->revision != revision_))
            return Err(sync_guard_error("sync session is stale or conflicted"_str));
        return Ok(empty {});
    }
    auto favorite_changed_after(i64 item) const -> bool override {
        if (token_ == u64()) return false;
        auto found = lock_->favorites.get(item);
        return lock_->favorite_overflow > revision_ || (found.is_some() && **found > revision_);
    }
    auto favorite(i64 item) -> void {
        if (lock_->active == u64()) return;
        if (lock_->favorites.len() < usize(200000))
            (void)lock_->favorites.insert(item, lock_->revision);
        else
            lock_->favorite_overflow = lock_->revision;
    }
    auto invalidate_sync() -> void { lock_->active = u64(); }
    auto finish_sync() -> void {
        if (token_ != u64() && lock_->active == token_) lock_->active = u64();
    }
    auto retain(Vec<i64> roots, Vec<i64> ids) -> ItemRetentionHandle {
        for (auto id : ids.as_slice()) {
            auto found = lock_->retained.get_mut(id);
            if (found.is_some())
                ++**found;
            else
                (void)lock_->retained.insert(id, usize(1));
        }
        return ItemRetentionHandle::make(state_.clone(), rstd::move(roots), rstd::move(ids));
    }
    auto retained_items() const -> Vec<i64> {
        Vec<i64> result;
        for (auto entry : lock_->retained.iter()) result.push(i64(*rstd::get<0>(entry)));
        return result;
    }
};
class ProviderSyncSession {
    ProviderStateHandle state_;
    ProjectionState     projection_;
    u64                 token_ {};
    u64                 revision_ {};

public:
    ProviderSyncSession(ProviderStateHandle state, ProjectionState projection, u64 token,
                        u64 revision)
        : state_(rstd::move(state)),
          projection_(rstd::move(projection)),
          token_(token),
          revision_(revision) {}
    ProviderSyncSession(const ProviderSyncSession&)                    = delete;
    auto operator=(const ProviderSyncSession&) -> ProviderSyncSession& = delete;
    ~ProviderSyncSession() { cancel(); }
    auto write(bool finalize = false) -> Result<Box<ProviderWriteGuard>, sqlito::Error> {
        auto projection = ProjectionWriteLock::make(projection_.clone());
        auto lock       = state_->lock().unwrap();
        if (lock->active != token_ || (finalize && lock->revision != revision_))
            return Err(sync_guard_error("sync session is stale or conflicted"_str));
        return Ok(Box<ProviderWriteGuard>::make(state_.clone(),
                                                Some(rstd::move(projection)),
                                                rstd::move(lock),
                                                token_,
                                                revision_,
                                                finalize));
    }
    auto cancel() -> void {
        auto state = state_->lock().unwrap();
        if (state->active == token_) state->active = u64();
    }
};
auto try_new_instance_id() -> Option<String> { return random_uuid().ok(); }
auto new_instance_id() -> String {
    return random_uuid().expect("cannot generate instance UUID"_str);
}
class SyncCoordinator {
    Arc<Mutex<CoordinatorState>>    state_ { Arc<Mutex<CoordinatorState>>::make() };
    Arc<Mutex<CoordinatorMetadata>> metadata_ { Arc<Mutex<CoordinatorMetadata>>::make(
        CoordinatorMetadata { new_instance_id() }) };
    ProjectionState                 projection_ { ProjectionState::make() };

    auto provider_state(i64 provider) -> ProviderStateHandle {
        auto state = state_->lock().unwrap();
        auto found = state->providers.get(provider);
        if (found.is_some()) return (*found)->clone();
        auto value = ProviderStateHandle::make();
        (void)state->providers.insert(provider, value.clone());
        return value;
    }

    static auto bump_epoch(CoordinatorMetadata& metadata, i64 owner) -> void {
        for (auto& entry : metadata.epochs) {
            if (rstd::get<0>(entry) == owner) {
                rstd::get<1>(entry) = rstd::get<1>(entry) + u64(1);
                return;
            }
        }
        metadata.epochs.push({ owner, u64(1) });
    }

public:
    SyncCoordinator() = default;
    auto clone() const -> SyncCoordinator {
        return SyncCoordinator(state_.clone(), metadata_.clone(), projection_.clone());
    }
    auto start(i64 provider) -> Result<Box<ProviderSyncSession>, sqlito::Error> {
        auto state = provider_state(provider);
        auto lock  = state->lock().unwrap();
        if (lock->active != u64()) return Err(sync_guard_error("full sync is already running"_str));
        if (lock->sequence == u64::MAX)
            return Err(sync_guard_error("sync generation exhausted"_str));
        lock->active = ++lock->sequence;
        lock->favorites.clear();
        lock->favorite_overflow = u64();
        return Ok(Box<ProviderSyncSession>::make(
            state.clone(), projection_.clone(), lock->active, lock->revision));
    }
    auto lock_writes() -> ProjectionWriteLock {
        return ProjectionWriteLock::make(projection_.clone());
    }
    auto write(i64 provider, ProjectionWriteLock projection)
        -> Result<Box<ProviderWriteGuard>, sqlito::Error> {
        auto state = provider_state(provider);
        auto lock  = state->lock().unwrap();
        if (lock->revision == u64::MAX)
            return Err(sync_guard_error("write revision exhausted"_str));
        auto revision = ++lock->revision;
        return Ok(Box<ProviderWriteGuard>::make(
            state.clone(), Some(rstd::move(projection)), rstd::move(lock), u64(), revision, false));
    }
    auto write(i64 provider) -> Result<Box<ProviderWriteGuard>, sqlito::Error> {
        return write(provider, lock_writes());
    }
    auto retain_guard(i64 provider) -> Box<ProviderWriteGuard> {
        auto state    = provider_state(provider);
        auto lock     = state->lock().unwrap();
        auto revision = lock->revision;
        return Box<ProviderWriteGuard>::make(
            state.clone(), None(), rstd::move(lock), u64(), revision, false);
    }
    auto read_snapshot() const -> ProjectionSnapshot {
        auto                 metadata = metadata_->lock().unwrap();
        Vec<tuple<i64, u64>> epochs;
        for (const auto& entry : metadata->epochs.as_slice())
            epochs.push({ rstd::get<0>(entry), rstd::get<1>(entry) });
        return { metadata->instance.clone(), metadata->revision, rstd::move(epochs) };
    }
    auto lock_snapshot() const -> ProjectionRead {
        auto lock = projection_->write().unwrap();
        return { projection_.clone(), rstd::move(lock), read_snapshot() };
    }
    auto bump() -> void {
        auto metadata      = metadata_->lock().unwrap();
        metadata->revision = metadata->revision + u64(1);
    }
    auto epoch(i64 owner) const -> u64 {
        auto metadata = metadata_->lock().unwrap();
        for (const auto& entry : metadata->epochs.as_slice())
            if (rstd::get<0>(entry) == owner) return rstd::get<1>(entry);
        return u64();
    }
    auto write_revision(i64 provider) -> u64 {
        auto state = provider_state(provider);
        auto lock  = state->lock().unwrap();
        return lock->revision;
    }
    auto invalidate(i64 owner) -> void {
        auto metadata = metadata_->lock().unwrap();
        bump_epoch(*metadata, owner);
        if (owner != i64()) bump_epoch(*metadata, i64());
        metadata->revision = metadata->revision + u64(1);
    }

private:
    SyncCoordinator(Arc<Mutex<CoordinatorState>> state, Arc<Mutex<CoordinatorMetadata>> metadata,
                    ProjectionState projection)
        : state_(rstd::move(state)),
          metadata_(rstd::move(metadata)),
          projection_(rstd::move(projection)) {}
};
class WriteGeneration {
public:
    virtual ~WriteGeneration()                                 = default;
    virtual auto check() const -> Result<empty, sqlito::Error> = 0;
};
} // namespace qcm::backend
