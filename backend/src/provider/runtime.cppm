export module qcm.backend:provider.runtime;
export import rstd;
export import :provider.interface;
export import sqlito;
import :provider.registry;
export import :resource.identity;
export import :resource.response;
using alloc::rc::Rc;
using qcm::backend::resource_cache::ResourceKind;
using qcm::backend::resource_cache::Response;
using rstd::async::RuntimeHandle;
using rstd::collections::BTreeMap;
using rstd::sync::Arc;
template<class T>
using RuntimeWeak = alloc::rc::Weak<T>;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct ProviderStatusCheck {
    i64                     provider_id {};
    u64                     epoch {};
    ProviderGenerationCheck generation;

    auto clone() const -> ProviderStatusCheck { return { provider_id, epoch, generation.clone() }; }
};

struct ProviderStatusSnapshot {
    ProviderStatusCheck    check;
    ProviderCommonSnapshot common;
};

struct ProviderResourcePlan {
    ProviderHandle                   provider;
    resource_cache::PreparedResource preparation;
    i64                              provider_id {};
    ResourceKind                     kind { ResourceKind::Image };
    String                           representation;
    auto                             fork() const -> ProviderResult<ProviderResourcePlan>;
    auto replace_delivery_headers(const lihttpto::Headers& headers) -> ProviderResult<empty>;
    auto resource_scope() -> ProviderResult<String>;
    auto execute_async(bool streaming = true) && -> rstd::async::coro<ProviderResult<Response>>;
};
struct ProviderRuntimeEntry {
    i64                id {};
    String             type_name;
    ProviderHandle     instance;
    ProviderSyncConfig sync;
    Option<u64>        snapshot_epoch { None() };
    u64                snapshot_stamp {};
};
class ProviderSnapshot {
    RuntimeWeak<ProviderRuntimeEntry> owner_;
    ProviderGenerationCheck           generation_;
    i64                               provider_id_;
    ProviderSyncConfig                sync_;

public:
    ProviderHandle instance;
    u64            stamp;
    explicit ProviderSnapshot(const Rc<ProviderRuntimeEntry>& owner);
    auto clone() const -> ProviderSnapshot;
    auto is_current() const -> bool;
    auto context(sqlito::ExecutorHandle db, SyncCoordinator coordinator) const -> ProviderContext;

private:
    ProviderSnapshot(RuntimeWeak<ProviderRuntimeEntry> owner, ProviderGenerationCheck generation,
                     i64 id, ProviderSyncConfig sync, ProviderHandle provider, u64 epoch);
};
class ProviderRuntime {
    RuntimeHandle                           runtime_;
    BTreeMap<i64, Rc<ProviderRuntimeEntry>> entries_;
    u64                                     next_snapshot_stamp_ {};
    Arc<PlayingState>                       playing_;
    String                                  device_id_;
    Option<sqlito::ExecutorHandle>          state_db_;

    auto refresh_stamp(Rc<ProviderRuntimeEntry>& entry) -> u64;

public:
    explicit ProviderRuntime(RuntimeHandle runtime);
    auto create(const ProviderRegistry& registry, ProviderOptions options) const
        -> ProviderResult<ProviderHandle>;
    static auto load(sqlito::ExecutorHandle db, const ProviderRegistry& registry,
                     RuntimeHandle runtime_handle, Option<String> device_id = None(),
                     Option<sqlito::ExecutorHandle> state_db = None(),
                     ProviderRuntime* previous = nullptr, Option<i64> changed = None())
        -> rstd::async::coro<ProviderResult<ProviderRuntime>>;
    auto reload_one(sqlito::ExecutorHandle db, const ProviderRegistry& registry, i64 id)
        -> rstd::async::coro<ProviderResult<empty>>;
    auto playing_state() const -> Arc<PlayingState>;
    auto find(i64 provider) -> Provider*;
    auto acquire(i64 provider) -> ProviderHandle;
    auto snapshot(i64 provider) -> Option<ProviderSnapshot>;
    auto call_snapshot(i64 provider, const ProviderHandle& expected, Option<u64> epoch = None())
        -> Option<ProviderSnapshot>;
    auto status_snapshot() -> Vec<ProviderStatusSnapshot>;
    auto status_is_current(const ProviderStatusCheck& check) const -> bool;
    auto replace(i64 provider, ProviderHandle instance) -> bool;
    auto add(i64 provider, const ProviderMetaInfo& meta, ProviderHandle instance) -> void;
    auto invalidate_replaced_by(ProviderRuntime& replacement) -> void;
    auto close_async() -> rstd::async::coro<empty>;
    auto sync_config(i64 provider) const -> const ProviderSyncConfig*;
    auto len() const -> usize { return entries_.len(); }
    auto prepare_resource_async(i64 provider, Resource resource, const lihttpto::Headers& headers)
        -> rstd::async::coro<ProviderResult<ProviderResourcePlan>>;
    auto keep_playback_async(i64 provider, String state)
        -> rstd::async::coro<ProviderResult<empty>>;
    auto close_playback_async(i64 provider, String state)
        -> rstd::async::coro<ProviderResult<empty>>;
    auto open_playback_async(i64 provider, PlaybackRequest request)
        -> rstd::async::coro<ProviderResult<PreparedPlayback>>;
    auto sync_async(i64 provider, ProviderContext context, Option<Item> item = None())
        -> rstd::async::coro<ProviderResult<u32>>;
    auto share_url(i64 provider, ref<str> native_id, ItemType item_type)
        -> ProviderResult<Option<lihttpto::Url>>;
};
} // namespace qcm::backend
