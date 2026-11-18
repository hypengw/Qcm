export module qcm.backend:db.sync.run;
export import rstd;
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
    explicit ProjectionWriteLease(ProjectionState owner);
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

    ProjectionRead(ProjectionState owner, RwLockWriteGuard<empty> lock,
                   ProjectionSnapshot snapshot);
    ProjectionRead(const ProjectionRead&)                    = delete;
    auto operator=(const ProjectionRead&) -> ProjectionRead& = delete;
    ProjectionRead(ProjectionRead&&) noexcept                = default;
    auto operator=(ProjectionRead&&) -> ProjectionRead&      = delete;
};
auto sync_guard_error(ref<str> message) -> sqlito::Error;
class ItemRetention {
    ProviderStateHandle state_;
    Vec<i64>            roots_;
    Vec<i64>            ids_;

public:
    ItemRetention(ProviderStateHandle state, Vec<i64> roots, Vec<i64> ids);
    ItemRetention(const ItemRetention&)                    = delete;
    auto operator=(const ItemRetention&) -> ItemRetention& = delete;
    ~ItemRetention();
    auto roots() const -> slice<i64>;
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
                       MutexGuard<ProviderWriteState> lock, u64 token, u64 revision, bool finalize);
    ProviderWriteGuard(const ProviderWriteGuard&)                    = delete;
    auto operator=(const ProviderWriteGuard&) -> ProviderWriteGuard& = delete;
    ProviderWriteGuard(ProviderWriteGuard&&)                         = delete;
    auto operator=(ProviderWriteGuard&&) -> ProviderWriteGuard&      = delete;
    auto check_origin() const -> Result<empty, sqlito::Error> override;
    auto favorite_changed_after(i64 item) const -> bool override;
    auto favorite(i64 item) -> void;
    auto invalidate_sync() -> void;
    auto finish_sync() -> void;
    auto retain(Vec<i64> roots, Vec<i64> ids) -> ItemRetentionHandle;
    auto retained_items() const -> Vec<i64>;
};
class ProviderSyncSession {
    ProviderStateHandle state_;
    ProjectionState     projection_;
    u64                 token_ {};
    u64                 revision_ {};

public:
    ProviderSyncSession(ProviderStateHandle state, ProjectionState projection, u64 token,
                        u64 revision);
    ProviderSyncSession(const ProviderSyncSession&)                    = delete;
    auto operator=(const ProviderSyncSession&) -> ProviderSyncSession& = delete;
    ~ProviderSyncSession();
    auto write(bool finalize = false) -> Result<Box<ProviderWriteGuard>, sqlito::Error>;
    auto cancel() -> void;
};
auto try_new_instance_id() -> Option<String>;
auto new_instance_id() -> String;
class SyncCoordinator {
    Arc<Mutex<CoordinatorState>>    state_;
    Arc<Mutex<CoordinatorMetadata>> metadata_;
    ProjectionState                 projection_;

    auto provider_state(i64 provider) -> ProviderStateHandle;

    static auto bump_epoch(CoordinatorMetadata& metadata, i64 owner) -> void;

public:
    SyncCoordinator();
    auto clone() const -> SyncCoordinator;
    auto start(i64 provider) -> Result<Box<ProviderSyncSession>, sqlito::Error>;
    auto lock_writes() -> ProjectionWriteLock;
    auto write(i64 provider, ProjectionWriteLock projection)
        -> Result<Box<ProviderWriteGuard>, sqlito::Error>;
    auto write(i64 provider) -> Result<Box<ProviderWriteGuard>, sqlito::Error>;
    auto retain_guard(i64 provider) -> Box<ProviderWriteGuard>;
    auto read_snapshot() const -> ProjectionSnapshot;
    auto lock_snapshot() const -> ProjectionRead;
    auto bump() -> void;
    auto epoch(i64 owner) const -> u64;
    auto write_revision(i64 provider) -> u64;
    auto invalidate(i64 owner) -> void;

private:
    SyncCoordinator(Arc<Mutex<CoordinatorState>> state, Arc<Mutex<CoordinatorMetadata>> metadata,
                    ProjectionState projection);
};
class WriteGeneration {
public:
    virtual ~WriteGeneration()                                 = default;
    virtual auto check() const -> Result<empty, sqlito::Error> = 0;
};

using ProviderWriteGuards = Vec<Box<ProviderWriteGuard>>;
auto write_items_locked(SyncCoordinator& coordinator, sqlito::Connection& db, slice<i64> ids,
                        ProjectionWriteLock projection)
    -> Result<ProviderWriteGuards, sqlito::Error>;
auto write_items(SyncCoordinator& coordinator, sqlito::Connection& db, slice<i64> ids)
    -> Result<ProviderWriteGuards, sqlito::Error>;
auto write_mixes(SyncCoordinator& coordinator, sqlito::Connection& db, slice<i64> mix_ids,
                 slice<i64> item_ids) -> Result<ProviderWriteGuards, sqlito::Error>;
auto retain_items(SyncCoordinator& coordinator, sqlito::Connection& db, i64 provider,
                  slice<i64> requested) -> Result<ItemRetentionHandle, sqlito::Error>;
} // namespace qcm::backend
