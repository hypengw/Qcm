module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:provider.runtime;
export import rstd;
export import :provider.interface;
export import sqlito;
import :provider.registry;
export import :resource.identity;
export import :resource.response;
import :db.database;
import :db.provider.load;
import :db.error.provider;
import rstd.log;
import rstd.json;
using alloc::rc::Rc;
using qcm::backend::resource_cache::ResourceKind;
using qcm::backend::resource_cache::Response;
using rstd::async::RuntimeHandle;
using rstd::collections::BTreeMap;
using rstd::sync::Arc;
template<class T>
using RuntimeWeak = alloc::rc::Weak<T>;
using alloc::rc::make_rc;
using namespace rstd::prelude;
using namespace rstd::literals;

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
    auto                             fork() const -> ProviderResult<ProviderResourcePlan> {
        if (! preparation.is_valid()) return Err(ProviderError::NotAuth());
        auto instance = provider.clone();
        auto copy     = preparation.clone();
        return Ok(ProviderResourcePlan {
            rstd::move(instance), rstd::move(copy), provider_id, kind, representation.clone() });
    }
    auto replace_delivery_headers(const lihttpto::Headers& headers) -> ProviderResult<empty> {
        return preparation.replace_delivery_headers(headers);
    }
    auto resource_scope() -> ProviderResult<String> {
        if (! preparation.is_valid()) return Err(ProviderError::NotAuth());
        auto              snapshot = provider->status_snapshot();
        auto              identity = rstd_try(provider.get()->resource_scope());
        rstd::json::Array scope;
        scope.push(rstd::json::Value::String(rstd::move(snapshot.type_name)));
        scope.push(rstd::json::Value::String(rstd::move(snapshot.auth.server_url)));
        if (identity.is_some()) {
            scope.push(rstd::json::Value::String(rstd::move(*identity)));
        } else {
            scope.push(rstd::json::Value::String(rstd_try(provider.get()->save_session())));
            scope.push(rstd::json::Value::String(rstd_try(provider.get()->save_cookie())));
        }
        scope.push(rstd::json::Value::String(representation.clone()));
        if (! preparation.is_valid()) return Err(ProviderError::NotAuth());
        return Ok(rstd::json::to_string(rstd::json::Value::Array(rstd::move(scope))));
    }
    auto execute_async(bool streaming = true) && -> rstd::async::coro<ProviderResult<Response>> {
        return rstd::move(preparation).execute(streaming);
    }
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
    explicit ProviderSnapshot(const Rc<ProviderRuntimeEntry>& owner)
        : owner_(owner.downgrade()),
          generation_(owner->instance->generation_check()),
          provider_id_(owner->id),
          sync_(owner->sync.clone()),
          instance(owner->instance.clone()),
          stamp(owner->snapshot_stamp) {}
    auto clone() const -> ProviderSnapshot {
        return ProviderSnapshot(owner_.clone(),
                                generation_.clone(),
                                provider_id_,
                                sync_.clone(),
                                instance.clone(),
                                stamp);
    }
    auto is_current() const -> bool {
        auto owner = owner_.upgrade();
        return owner.is_some() && ProviderHandle::ptr_eq((*owner)->instance, instance) &&
               (*owner)->snapshot_stamp == stamp && generation_.is_current();
    }
    auto context(sqlito::ExecutorHandle db, SyncCoordinator coordinator) const -> ProviderContext {
        ProviderContext result(
            rstd::move(db), rstd::move(coordinator), provider_id_, sync_.clone());
        result.set_valid([call = clone()] {
            return call.is_current();
        });
        return result;
    }

private:
    ProviderSnapshot(RuntimeWeak<ProviderRuntimeEntry> owner, ProviderGenerationCheck generation,
                     i64 id, ProviderSyncConfig sync, ProviderHandle provider, u64 epoch)
        : owner_(rstd::move(owner)),
          generation_(rstd::move(generation)),
          provider_id_(id),
          sync_(rstd::move(sync)),
          instance(rstd::move(provider)),
          stamp(epoch) {}
};
class ProviderRuntime {
    RuntimeHandle                           runtime_;
    BTreeMap<i64, Rc<ProviderRuntimeEntry>> entries_;
    u64                                     next_snapshot_stamp_ {};
    Arc<PlayingState>                       playing_ { Arc<PlayingState>::make() };
    String                                  device_id_;
    Option<sqlito::ExecutorHandle>          state_db_;

    auto refresh_stamp(Rc<ProviderRuntimeEntry>& entry) -> u64 {
        auto epoch = entry->instance.get()->lifecycle_epoch();
        if (entry->snapshot_epoch.is_none() || *entry->snapshot_epoch != epoch) {
            if (next_snapshot_stamp_ == u64::MAX)
                rstd::panic { "provider snapshot stamp exhausted" };
            entry.get()->snapshot_stamp = ++next_snapshot_stamp_;
            entry.get()->snapshot_epoch = Some(epoch);
        }
        return entry->snapshot_stamp;
    }

public:
    explicit ProviderRuntime(RuntimeHandle runtime): runtime_(rstd::move(runtime)) {}
    auto create(const ProviderRegistry& registry, ProviderOptions options) const
        -> ProviderResult<ProviderHandle> {
        options.device_id = device_id_.clone();
        options.playing   = playing_.clone();
        options.runtime   = Some(runtime_.clone());
        if (state_db_.is_some()) {
            auto runtime         = rstd_try(PluginRuntime::current());
            options.installation = Some(rstd_try(PluginContext::create(state_db_->clone(),
                                                                       options.type_name.clone(),
                                                                       device_id_.clone(),
                                                                       rstd::move(runtime)),
                                                 ProviderError::internal(rstd::move(_e))));
        }
        return registry.create(rstd::move(options));
    }
    static auto load(sqlito::ExecutorHandle db, const ProviderRegistry& registry,
                     RuntimeHandle runtime_handle, Option<String> device_id = None(),
                     Option<sqlito::ExecutorHandle> state_db = None(),
                     ProviderRuntime* previous = nullptr, Option<i64> changed = None())
        -> rstd::async::coro<ProviderResult<ProviderRuntime>> {
        auto rows =
            rstd_co_try(co_await database_run(db.clone(), [changed](sqlito::Connection& db) {
                return provider_load_rows(db, changed);
            }));
        ProviderRuntime runtime(rstd::move(runtime_handle));
        if (previous != nullptr) {
            runtime.next_snapshot_stamp_ = previous->next_snapshot_stamp_;
            runtime.playing_             = previous->playing_.clone();
            runtime.device_id_           = previous->device_id_.clone();
            runtime.state_db_ =
                previous->state_db_.is_some() ? Some(previous->state_db_->clone()) : None();
        }
        if (device_id.is_some()) runtime.device_id_ = rstd::move(*device_id);
        if (state_db.is_some()) runtime.state_db_ = Some(rstd::move(*state_db));
        for (auto& row : rows) {
            const auto* meta = registry.find(row.type_name.as_str());
            if (meta == nullptr) continue;
            auto provider_id = row.id;
            if (previous != nullptr && (changed.is_none() || *changed != provider_id)) {
                auto old = previous->entries_.get(provider_id);
                if (old.is_some() && (*old)->get()->type_name == row.type_name) {
                    runtime.entries_.insert(provider_id, (*old)->clone());
                    continue;
                }
            }
            ProviderOptions options;
            options.type_name = meta->type_name.clone();
            options.name      = row.name.clone();
            options.base_url  = row.base_url.clone();
            options.id        = Some(provider_id);
            if (row.auth.is_some() && ! row.auth->is_empty()) {
                auto auth = rstd::json::decode<AuthMethod>(row.auth->as_str());
                if (auth.is_err()) {
                    rstd::log::warn("cannot decode provider {} authentication: {}",
                                    provider_id,
                                    auth.as_ref().unwrap_err());
                    continue;
                }
                options.auth_method = Some(rstd::move(*auth));
            }
            auto instance = runtime.create(registry, rstd::move(options));
            if (instance.is_err()) {
                rstd::log::warn("cannot initialize provider {}: {}",
                                provider_id,
                                instance.as_ref().unwrap_err());
                continue;
            }
            auto created = rstd::move(*instance);
            created.get()->set_session_store(db.clone(), provider_id, rstd::move(row.session_key));
            if (row.cookie.is_some() && ! row.cookie->is_empty()) {
                auto restored_cookie = created.get()->load_cookie(row.cookie->as_str());
                if (restored_cookie.is_err())
                    rstd::log::warn("cannot restore provider {} cookies: {}",
                                    provider_id,
                                    restored_cookie.as_ref().unwrap_err());
            }
            auto loaded = created.get()->load_session(row.custom.as_str());
            if (! row.custom.is_empty() && loaded.is_err()) {
                rstd::log::warn("cannot restore provider {} session: {}",
                                provider_id,
                                loaded.as_ref().unwrap_err());
                continue;
            }
            SettingsValues settings;
            if (row.settings.is_some()) {
                auto parsed = rstd::json::decode<SettingsValues>(row.settings->as_str());
                if (parsed.is_err()) {
                    rstd::log::warn("cannot decode provider {} settings: {}",
                                    provider_id,
                                    parsed.as_ref().unwrap_err());
                    continue;
                }
                settings = rstd::move(*parsed);
            }
            auto normalized = created.get()->validate_settings(settings);
            if (normalized.is_err()) {
                rstd::log::warn("cannot normalize provider {} settings: {}",
                                provider_id,
                                normalized.as_ref().unwrap_err());
                continue;
            }
            auto applied = created.get()->set_settings(
                SettingsSnapshot { row.settings_revision, rstd::move(*normalized) });
            if (applied.is_err()) {
                rstd::log::warn("cannot restore provider {} settings: {}",
                                provider_id,
                                applied.as_ref().unwrap_err());
                continue;
            }
            runtime.entries_.insert(
                provider_id,
                make_rc<ProviderRuntimeEntry>(ProviderRuntimeEntry { provider_id,
                                                                     meta->type_name.clone(),
                                                                     rstd::move(created),
                                                                     meta->sync.clone() }));
        }
        co_return Ok(rstd::move(runtime));
    }
    auto reload_one(sqlito::ExecutorHandle db, const ProviderRegistry& registry, i64 id)
        -> rstd::async::coro<ProviderResult<empty>> {
        auto fresh = rstd_co_try(co_await load(
            rstd::move(db), registry, runtime_.clone(), None(), None(), this, Some(id)));
        auto entry = fresh.entries_.remove(id);
        if (entry.is_some()) {
            entries_.insert(id, rstd::move(*entry));
        } else {
            (void)entries_.remove(id);
        }
        co_return Ok(empty {});
    }
    auto playing_state() const -> Arc<PlayingState> { return playing_.clone(); }
    auto find(i64 provider) -> Provider* {
        auto entry = entries_.get_mut(provider);
        return entry.is_some() ? (*entry)->get()->instance.get() : nullptr;
    }
    auto acquire(i64 provider) -> ProviderHandle {
        auto entry = entries_.get(provider);
        return entry.is_some() ? (*entry)->get()->instance.clone() : ProviderHandle {};
    }
    auto snapshot(i64 provider) -> Option<ProviderSnapshot> {
        auto entry = entries_.get_mut(provider);
        if (entry.is_none()) return None();
        (void)refresh_stamp(**entry);
        return Some(ProviderSnapshot(**entry));
    }
    auto call_snapshot(i64 provider, const ProviderHandle& expected, Option<u64> epoch = None())
        -> Option<ProviderSnapshot> {
        auto call = snapshot(provider);
        if (call.is_none() || ! ProviderHandle::ptr_eq(call->instance, expected) ||
            ! call->is_current())
            return None();
        if (epoch.is_some() && call->instance->session_epoch() != epoch) return None();
        return call;
    }
    auto status_snapshot() -> Vec<ProviderStatusSnapshot> {
        Vec<ProviderStatusSnapshot> statuses;
        for (auto entry : entries_.values_mut()) {
            auto& current    = *entry->get();
            auto  stamp      = refresh_stamp(*entry);
            auto  generation = current.instance->generation_check();
            if (! generation.is_current()) continue;
            statuses.push(ProviderStatusSnapshot { { current.id, stamp, rstd::move(generation) },
                                                   current.instance->status_snapshot() });
        }
        return statuses;
    }
    auto status_is_current(const ProviderStatusCheck& check) const -> bool {
        auto entry = entries_.get(check.provider_id);
        if (entry.is_none() || ! check.generation.is_current()) return false;
        const auto& current = *(*entry)->get();
        return current.snapshot_stamp == check.epoch &&
               current.snapshot_epoch == Some(current.instance->lifecycle_epoch()) &&
               check.generation.same_generation(current.instance->generation_check());
    }
    auto replace(i64 provider, ProviderHandle instance) -> bool {
        auto found = entries_.get_mut(provider);
        if (found.is_none()) return false;
        auto& entry = **found;
        // Service drains radio/playback before invalidating the detached provider.
        if (! ProviderHandle::ptr_eq(entry->instance, instance))
            entry.get()->snapshot_epoch = None();
        entry.get()->instance = rstd::move(instance);
        (void)refresh_stamp(entry);
        return true;
    }
    auto add(i64 provider, const ProviderMetaInfo& meta, ProviderHandle instance) -> void {
        auto entry = make_rc<ProviderRuntimeEntry>(ProviderRuntimeEntry {
            provider, meta.type_name.clone(), rstd::move(instance), meta.sync.clone() });
        auto old   = entries_.insert(provider, entry.clone());
        if (old.is_some() && ! ProviderHandle::ptr_eq((*old)->instance, entry->instance))
            old->get()->instance.get()->invalidate();
    }
    auto invalidate_replaced_by(ProviderRuntime& replacement) -> void {
        for (auto entry : entries_.values_mut())
            if (replacement.find(entry->get()->id) != entry->get()->instance.get())
                entry->get()->instance.get()->invalidate();
    }
    auto close_async() -> rstd::async::coro<empty> {
        Vec<ProviderHandle> providers;
        for (auto entry : entries_.values()) providers.push(entry->get()->instance.clone());
        for (auto& provider : providers) (void)co_await provider.get()->close_async();
        co_return empty {};
    }
    auto sync_config(i64 provider) const -> const ProviderSyncConfig* {
        auto entry = entries_.get(provider);
        return entry.is_some() ? rstd::addressof((*entry)->get()->sync) : nullptr;
    }
    auto len() const -> usize { return entries_.len(); }
    auto prepare_resource_async(i64 provider, Resource resource, const lihttpto::Headers& headers)
        -> rstd::async::coro<ProviderResult<ProviderResourcePlan>> {
        auto instance = acquire(provider);
        if (! instance) co_return Err(ProviderError::NotFound());
        auto kind           = resource.kind();
        auto representation = resource.representation();
        auto plan           = rstd_co_try(
            co_await instance.get()->prepare_resource_async(rstd::move(resource), headers));
        co_return Ok(ProviderResourcePlan {
            rstd::move(instance), rstd::move(plan), provider, kind, rstd::move(representation) });
    }
    auto keep_playback_async(i64 provider, String state)
        -> rstd::async::coro<ProviderResult<empty>> {
        auto instance = acquire(provider);
        if (! instance) co_return Err(ProviderError::NotFound());
        co_return co_await instance.get()->keep_playback_async(rstd::move(state));
    }
    auto close_playback_async(i64 provider, String state)
        -> rstd::async::coro<ProviderResult<empty>> {
        auto instance = acquire(provider);
        if (! instance) co_return Err(ProviderError::NotFound());
        co_return co_await instance.get()->close_playback_async(rstd::move(state));
    }
    auto open_playback_async(i64 provider, PlaybackRequest request)
        -> rstd::async::coro<ProviderResult<PreparedPlayback>> {
        auto instance = acquire(provider);
        if (! instance) co_return Err(ProviderError::NotFound());
        co_return co_await instance.get()->open_playback_async(rstd::move(request));
    }
    auto sync_async(i64 provider, ProviderContext context, Option<Item> item = None())
        -> rstd::async::coro<ProviderResult<u32>> {
        auto instance = acquire(provider);
        if (! instance) co_return Err(ProviderError::NotFound());
        co_return co_await instance.get()->sync_async(rstd::move(context), rstd::move(item));
    }
    auto share_url(i64 provider, ref<str> native_id, ItemType item_type)
        -> ProviderResult<Option<lihttpto::Url>> {
        auto* instance = find(provider);
        if (instance == nullptr) return Err(ProviderError::NotFound());
        return instance->share_url(native_id, item_type);
    }
};
} // namespace qcm::backend
