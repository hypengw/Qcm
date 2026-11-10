module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:service;
export import rstd;
export import sqlito;
export import :entity;
export import :event;
export import :playback;
export import :protocol;
export import :sync_events;
export import :task;
import lihttpto;
import :api.provider;
import :api.provider_status;
import :api.radio;
import :api.entity;
import :db;
import :db.provider.status;
import :db.query.media;
import :db.mix.refresh;
import :error;
import :error_response;
import :plugin;
import :provider;
import :resource;
import :sync_state;
template<class T>
using AsyncMutex = rstd::async::Mutex<T>;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::collections::HashMap;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::async::coro;
using rstd::async::RuntimeHandle;

export namespace qcm::backend
{
class QcmService {
    class RequestDispatcher;
    sqlito::ExecutorHandle                     db_;
    Option<sqlito::ExecutorHandle>             cache_db_;
    SyncCoordinator                            coordinator_;
    EntityQueries                              entity_queries_;
    Option<StateDatabase>                      state_db_;
    ProviderRegistry                           providers_;
    TmpProviderStore                           tmp_providers_;
    Arc<RadioSessions>                         radio_sessions_ { Arc<RadioSessions>::make() };
    Option<String>                             cache_dir_;
    Option<Arc<resource_cache::ResourceCache>> resource_store_;
    ResourceObjectFetch                        object_fetch_;
    Rc<ProviderRuntime>                        provider_runtime_;
    Weak<QcmService>                           self_ { Weak<QcmService>::make() };
    PlaybackSessions                           playback_;
    Box<MixRefresh>                            mix_refresh_ { Box<MixRefresh>::make() };
    Option<EventSink>                          event_sink_;
    SyncEvents                                 sync_events_;
    Rc<TaskOwner>                              tasks_ { TaskOwner::make() };
    HashMap<i64, Weak<AsyncMutex<empty>>>      provider_change_locks_;

public:
    QcmService(sqlito::ExecutorHandle db, ProviderRegistry providers, ProviderRuntime runtime,
               Option<sqlito::ExecutorHandle> cache_db, Option<String> cache_dir,
               Option<StateDatabase> state_db)
        : db_(rstd::move(db)),
          cache_db_(rstd::move(cache_db)),
          entity_queries_(db_.clone(), coordinator_.clone()),
          state_db_(rstd::move(state_db)),
          providers_(rstd::move(providers)),
          cache_dir_(rstd::move(cache_dir)),
          provider_runtime_(make_rc<ProviderRuntime>(rstd::move(runtime))),
          playback_(db_.clone(), provider_runtime_.clone()) {}
    static auto
    make(sqlito::ExecutorHandle db, RuntimeHandle runtime_handle, ProviderRegistry providers = {},
         Option<sqlito::ExecutorHandle> cache_db = None(), Option<String> cache_dir = None(),
         Option<String> device_id = None(), Option<String> state_db_path = None())
        -> coro<Result<Arc<QcmService>, ProcessError>> {
        // Cancellation drops this owner, stopping and joining its database worker.
        Option<StateDatabase>          state_db;
        Option<sqlito::ExecutorHandle> state_handle;
        if (state_db_path.is_some()) {
            state_db    = Some(rstd_co_try(co_await StateDatabase::open(rstd::move(*state_db_path)),
                                           ProcessError::internal(rstd::move(_e))));
            auto handle = state_db->handle();
            if (handle.is_err()) {
                auto error = ProcessError::internal(rstd::move(handle).unwrap_err());
                (void)co_await state_db->shutdown();
                co_return Err(rstd::move(error));
            }
            state_handle = Some(rstd::move(*handle));
        }
        auto runtime =
            co_await ProviderRuntime::load(db.clone(),
                                           providers,
                                           rstd::move(runtime_handle),
                                           device_id.is_some() ? Some(device_id->clone()) : None(),
                                           rstd::move(state_handle));
        if (runtime.is_err()) {
            auto error = rstd::into<ProcessError>(rstd::move(runtime).unwrap_err());
            if (state_db.is_some()) (void)co_await state_db->shutdown();
            co_return Err(rstd::move(error));
        }
        auto service   = Arc<QcmService>::make(rstd::move(db),
                                               rstd::move(providers),
                                               rstd::move(*runtime),
                                               rstd::move(cache_db),
                                               rstd::move(cache_dir),
                                               rstd::move(state_db));
        service->self_ = service.downgrade();
        service->start_sync_events();
        co_return Ok(rstd::move(service));
    }

    auto set_event_sink(Option<EventSink> sink) -> void {
        sync_events_.set_event_sink(sink.is_some() ? Some(sink->clone()) : None());
        event_sink_ = rstd::move(sink);
    }
    static auto populate_sync_event(Weak<QcmService> weak, QcmMessage message)
        -> coro<ProcessResult<QcmMessage>> {
        if (auto service = weak.upgrade())
            co_return co_await service->ensure_entities(rstd::move(message));
        co_return Err(ProcessError::internal("sync event service is unavailable"_str));
    }
    static auto refresh_sync_providers(Weak<QcmService> weak) -> coro<ProcessResult<QcmMessage>> {
        if (auto service = weak.upgrade()) co_return co_await service->provider_status();
        co_return Err(ProcessError::internal("sync event service is unavailable"_str));
    }
    auto start_sync_events() -> void {
        sync_events_.start(
            [weak = self_.clone()](QcmMessage message) {
                return populate_sync_event(weak.clone(), rstd::move(message));
            },
            [weak = self_.clone()] {
                return refresh_sync_providers(weak.clone());
            });
    }
    auto sync_publisher(i64 provider_id, const ProviderHandle& expected = {})
        -> Option<SyncPublisher> {
        return sync_events_.publisher(*provider_runtime_.get(), provider_id, expected);
    }
    auto flush_sync_events() -> coro<empty> { return sync_events_.flush(); }
    auto start_resource_cache() -> coro<ProcessResult<empty>> {
        if (cache_dir_.is_none() || resource_store_.is_some()) co_return Ok(empty {});
        auto opened = rstd_co_try(
            co_await resource_cache::ResourceCache::open(PathBuf::from(cache_dir_->as_str())));
        resource_store_ = Some(rstd::move(opened));
        co_return Ok(empty {});
    }
    auto stop_resource_cache() -> coro<empty> {
        if (resource_store_.is_some()) {
            if ((co_await (*resource_store_)->shutdown()).is_err())
                rstd::log::warn("resource cache shutdown failed");
            resource_store_ = None();
        }
        co_return empty {};
    }
    auto invalidate_resource_cache(i64 provider) -> coro<empty> {
        if (resource_store_.is_some() &&
            (co_await (*resource_store_)->invalidate(provider)).is_err())
            rstd::log::warn("resource cache invalidation failed for provider {}", provider);
        co_return empty {};
    }
    auto authorize_file_client(ref<str> key) -> Result<empty, resource_cache::Error> {
        if (resource_store_.is_none())
            return Err(resource_cache::Error::state("resource cache is unavailable"_str));
        return (*resource_store_)->authorize_file_client(key);
    }
    auto file_lease_response(ref<str> method, ref<str> path, const lihttpto::Headers& headers)
        -> coro<Option<resource_cache::Response>> {
        Option<Arc<resource_cache::ResourceCache>> cache;
        if (resource_store_.is_some()) cache = Some((*resource_store_).clone());
        return resource_cache::ResourceCache::file_lease_response(
            rstd::move(cache), method, path, headers);
    }
    auto temporary_provider(ref<str> key) -> TmpProviderHandle {
        return tmp_providers_.acquire(key);
    }
    auto provider_instance(i64 id) -> ProviderHandle {
        return provider_runtime_.get()->acquire(id);
    }
    auto complete_provider_mutation(Arc<QcmService> service, ProviderCommittedMutation& mutation)
        -> coro<ProcessResult<empty>> {
        auto                 old       = mutation.install(*provider_runtime_.get(), db_.clone());
        ProcessResult<empty> completed = Ok(empty {});
        if (mutation.kind == ProviderMutationKind::Deleted)
            completed = co_await reload_provider_async(mutation.provider_id);
        else if (mutation.replaces_instance())
            co_await finish_provider_replacement(mutation.provider_id, rstd::move(old));
        if (mutation.replaces_instance()) co_await invalidate_resource_cache(mutation.provider_id);
        auto event = [&] {
            switch (mutation.kind) {
            case ProviderMutationKind::Added:
                return BackendEvent::NewProvider(mutation.provider_id);
            case ProviderMutationKind::Updated:
                return BackendEvent::UpdateProvider(mutation.provider_id);
            case ProviderMutationKind::Replaced:
                return BackendEvent::ReplaceProvider(mutation.provider_id);
            case ProviderMutationKind::Deleted:
                return BackendEvent::DeleteProvider(mutation.provider_id);
            }
        }();
        co_await process_backend_event(service.clone(), rstd::move(event));
        co_return completed;
    }
    static auto run_sync_task(Arc<QcmService> service, i64 provider_id, ProviderHandle instance)
        -> coro<empty> {
        auto publisher = service->sync_publisher(provider_id, instance);
        auto result =
            co_await service->sync_provider_async(provider_id, None(), rstd::move(instance));
        if (result.is_err())
            rstd::log::error(
                "provider {} sync failed: {}", provider_id, result.as_ref().unwrap_err());
        if (publisher.is_some()) (void)publisher->refresh_providers();
        co_return empty {};
    }
    auto start_sync_task(Arc<QcmService> service, i64 provider_id) -> Result<i64, ProcessError> {
        if (provider_runtime_.get()->find(provider_id) == nullptr)
            return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
        auto instance = provider_runtime_.get()->acquire(provider_id);
        return tasks_.get()->start_sync(
            provider_id,
            instance.clone(),
            run_sync_task(service.clone(), provider_id, instance.clone()));
    }
    auto cancel_sync_tasks() -> void {
        sync_events_.begin_stop();
        tasks_.get()->cancel_sync();
    }
    static auto drain_sync_tasks(Rc<TaskOwner> tasks, coro<empty> events_stopped) -> coro<empty> {
        co_await tasks.get()->stop_sync();
        co_await rstd::move(events_stopped);
        co_return empty {};
    }
    auto stop_sync_tasks() -> coro<empty> {
        cancel_sync_tasks();
        return drain_sync_tasks(tasks_.clone(), sync_events_.stop());
    }
    auto provider_change_lock(i64 id) -> Arc<AsyncMutex<empty>> {
        Vec<i64> expired;
        for (auto entry : provider_change_locks_.iter())
            if (rstd::get<1>(entry)->strong_count() == usize())
                expired.push(i64(*rstd::get<0>(entry)));
        for (auto key : expired) provider_change_locks_.remove(key);
        if (auto entry = provider_change_locks_.get(id))
            if (auto live = (*entry)->upgrade()) return live;
        auto lock = Arc<AsyncMutex<empty>>::make(empty {});
        provider_change_locks_.insert(id, lock.downgrade());
        return lock;
    }
    auto begin_provider_shutdown() -> void { tasks_.get()->begin_provider_shutdown(); }
    auto stop_provider_changes() -> coro<empty> { return tasks_.get()->stop_provider_changes(); }
    auto finish_provider_replacement(i64 changed, ProviderHandle old) -> coro<empty> {
        sync_events_.refresh_sources();
        co_await tasks_.get()->cancel_replaced_requests(*provider_runtime_.get());
        if (old && provider_runtime_.get()->find(changed) != old.get()) {
            old.get()->cancel_streams();
            playback_.cancel(old.clone());
        }
        (void)co_await tasks_.get()->cancel_replaced_sync(*provider_runtime_.get());
        if (! old || provider_runtime_.get()->find(changed) == old.get()) co_return empty {};
        struct InvalidateOnDrop {
            ProviderHandle provider;
            ~InvalidateOnDrop() { provider.get()->invalidate(); }
        } invalidate { old.clone() };
        co_await radio_sessions_->invalidate(
            *provider_runtime_.get(), coordinator_, db_.clone(), old.clone());
        co_await playback_.invalidate(old.clone());
        (void)co_await old.get()->close_async();
        co_return empty {};
    }
    auto stop_providers() -> coro<empty> {
        co_await sync_events_.stop();
        co_await radio_sessions_->shutdown(*provider_runtime_.get(), coordinator_, db_.clone());
        (void)co_await tmp_providers_.close_async();
        (void)co_await provider_runtime_.get()->close_async();
        if (state_db_.is_some()) {
            auto database = state_db_.take().unwrap();
            if ((co_await database.shutdown()).is_err())
                rstd::log::warn("plugin state database shutdown failed");
        }
        co_return empty {};
    }
    auto reload_provider_async(i64 changed) -> coro<Result<empty, ProcessError>> {
        auto old = provider_runtime_.get()->acquire(changed);
        rstd_co_try(co_await provider_runtime_.get()->reload_one(db_.clone(), providers_, changed));
        co_await finish_provider_replacement(changed, rstd::move(old));
        co_return Ok(empty {});
    }
    auto owns_sync_status(i64 id, const ProviderHandle& instance, u64 lifecycle) -> bool {
        return provider_runtime_.get()->find(id) == instance.get() &&
               instance->lifecycle_epoch() == lifecycle;
    }
    auto sync_provider_async(i64 provider_id, Option<Item> item = None(),
                             ProviderHandle expected = {}) -> coro<Result<u32, ProcessError>> {
        auto instance =
            expected ? rstd::move(expected) : provider_runtime_.get()->acquire(provider_id);
        if (! instance || provider_runtime_.get()->find(provider_id) != instance.get())
            co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
        auto lifecycle = instance->lifecycle_epoch();
        auto publisher = sync_publisher(provider_id, instance);
        if (publisher.is_none()) co_return Err(ProcessError::NotAuth());
        if (item.is_none())
            (void)co_await publisher->send(SyncCommit::SetState(SyncState::Syncing));
        if (! owns_sync_status(provider_id, instance, lifecycle))
            co_return Err(ProcessError::NotAuth());
        auto epoch = instance->session_epoch();
        if (epoch.is_none()) {
            if (item.is_none())
                (void)co_await publisher->send(SyncCommit::SetState(SyncState::NotAuth));
            co_return Err(ProcessError::NotAuth());
        }
        auto call = provider_runtime_.get()->call_snapshot(provider_id, instance, Some(*epoch));
        if (call.is_none()) co_return Err(ProcessError::NotAuth());
        auto context = call->context(db_.clone(), coordinator_.clone());
        context.set_progress([publisher = publisher->clone()](SyncProgressKind kind, i32 count) {
            return publisher.progress(kind, count);
        });
        bool full_sync = item.is_none();
        auto result    = co_await instance.get()->sync_async(context.clone(), rstd::move(item));
        if (! context.is_valid()) {
            if (full_sync && provider_runtime_.get()->find(provider_id) == instance.get())
                (void)co_await publisher->send(SyncCommit::SetState(SyncState::NotAuth));
            co_return Err(ProcessError::NotAuth());
        }
        if (full_sync) {
            auto state = result.is_err() ? rstd::into<SyncState>(dyn<rstd::error::Error>::from_ref(
                                               result.as_ref().unwrap_err()))
                                         : SyncState::Finished;
            (void)co_await publisher->send(SyncCommit::SetState(state));
            if (! context.is_valid()) co_return Err(ProcessError::NotAuth());
        }
        if (result.is_err())
            co_return Err(ProcessError::from_provider(rstd::move(result).unwrap_err()));
        co_return Ok(*result);
    }
    auto sync_item_provider_async(i64 item_id) -> coro<Result<u32, ProcessError>> {
        auto item = rstd_co_try(co_await database_run(
            db_.clone(), [item_id](sqlito::Connection& db) -> ProcessResult<Option<Item>> {
                return Ok(rstd_try(read_item_by_id(db, item_id)));
            }));
        if (item.is_none()) co_return Err(ProcessError::NotFound());
        auto provider_id = item->provider_id;
        auto publisher   = sync_publisher(provider_id);
        auto result      = co_await sync_provider_async(provider_id, Some(rstd::move(*item)));
        if (publisher.is_some()) (void)publisher->refresh_providers();
        co_return rstd::move(result);
    }
    auto refresh_mix_async(i64 mix_id, bool force, ProviderRequestOwnerHandle owner = {})
        -> coro<Result<empty, ProcessError>> {
        co_return co_await mix_refresh_->refresh(
            db_.clone(),
            mix_id,
            force,
            [this,
             owner = rstd::move(owner)](Item item) mutable -> coro<Result<empty, ProcessError>> {
                auto provider_id = i64(item.provider_id);
                auto instance    = provider_runtime_.get()->acquire(provider_id);
                if (! instance)
                    co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
                if (owner) {
                    owner.get()->provider_id = provider_id;
                    owner.get()->instance    = instance.clone();
                }
                auto synced = co_await sync_provider_async(
                    provider_id, Some(rstd::move(item)), rstd::move(instance));
                if (owner) owner.get()->instance.reset();
                rstd_co_try(synced);
                co_return Ok(empty {});
            });
    }
    auto image_resource_plan(i64 id, ref<str> item_type_name, ref<str> image_type_name,
                             const lihttpto::Headers& headers)
        -> coro<Result<Option<ProviderResourcePlan>, ProcessError>> {
        auto descriptor = rstd_co_try(co_await database_run(
            db_.clone(),
            [id,
             item_type  = String::make(item_type_name),
             image_type = String::make(image_type_name)](sqlito::Connection& db) {
                return image_resource_descriptor(db, id, item_type.as_str(), image_type.as_str());
            }));
        if (descriptor.is_none()) co_return Ok(None<ProviderResourcePlan>());
        auto plan = rstd_co_try(co_await provider_runtime_.get()->prepare_resource_async(
            descriptor->provider_id,
            Resource::Image(rstd::move(descriptor->native_id),
                            rstd::move(descriptor->image_native),
                            descriptor->image_type),
            headers));
        co_return Ok(Some(rstd::move(plan)));
    }
    auto object_resource(ProviderResourcePlan plan, lihttpto::Headers headers)
        -> coro<Result<resource_cache::ResourceDelivery, resource_cache::Error>> {
        co_return co_await object_fetch_.respond(
            resource_store_.is_some() ? resource_store_->as_ptr().as_raw_ptr() : nullptr,
            rstd::move(plan),
            rstd::move(headers));
    }
    auto audio_resource_plan(i64 id, const lihttpto::Headers& headers)
        -> coro<Result<Option<ProviderResourcePlan>, ProcessError>> {
        auto descriptor =
            rstd_co_try(co_await database_run(db_.clone(), [id](sqlito::Connection& db) {
                return audio_resource_descriptor(db, id);
            }));
        if (descriptor.is_none()) co_return Ok(None<ProviderResourcePlan>());
        auto plan = rstd_co_try(co_await provider_runtime_.get()->prepare_resource_async(
            descriptor->provider_id, Resource::Audio(rstd::move(descriptor->native_id)), headers));
        co_return Ok(Some(rstd::move(plan)));
    }
    auto stop_radio_service() -> coro<empty> {
        return radio_sessions_->shutdown(*provider_runtime_.get(), coordinator_, db_.clone());
    }
    auto collect_radio_sessions() -> coro<empty> {
        return radio_sessions_->collect(*provider_runtime_.get(), coordinator_, db_.clone());
    }
    auto playback() -> PlaybackSessions& { return playback_; }
    auto resolve_playback_async(String token, usize index, String path, String query,
                                lihttpto::Headers headers)
        -> coro<ProcessResult<Option<resource_cache::ResourceDelivery>>> {
        co_return co_await playback_.resolve_resource(rstd::move(token),
                                                      index,
                                                      rstd::move(path),
                                                      rstd::move(query),
                                                      rstd::move(headers),
                                                      resource_store_.clone());
    }
    auto broadcast(QcmMessage message) -> coro<Result<empty, ProcessError>> {
        if (event_sink_.is_some())
            co_return co_await event_sink_->send_message(rstd::move(message));
        co_return Ok(empty {});
    }
    auto provider_meta_status() -> Option<QcmMessage> {
        if (providers_.metas().is_empty()) return None();
        auto rsp = Box<p_qcm::p_msg::M_ProviderMetaStatusMsg>::make();
        for (const auto& meta : providers_.metas())
            rsp->f_metas.push(rstd::into<Box<p_qcm::p_msg::p_model::M_ProviderMeta>>(
                ref<ProviderMetaInfo>::from_raw_parts(rstd::addressof(meta))));
        rsp->f_full  = true;
        auto message = qcm_message(i32(), rstd::move(rsp));
        return Some(rstd::move(message));
    }
    auto provider_status() -> coro<Result<QcmMessage, ProcessError>> {
        auto snapshots = provider_runtime_.get()->status_snapshot();
        auto libraries = rstd_co_try(co_await database_run(db_.clone(), [](sqlito::Connection& db) {
            return provider_status_libraries(db);
        }));
        snapshots.retain([&](const auto& snapshot) {
            return provider_runtime_.get()->status_is_current(snapshot.check);
        });
        HashMap<i64, ProviderStatusCheck> checks;
        for (const auto& snapshot : snapshots.as_slice())
            checks.insert(snapshot.check.provider_id, snapshot.check.clone());
        Vec<ProviderSyncStatus> statuses;
        for (const auto& status : sync_events_.snapshot())
            statuses.push(ProviderSyncStatus {
                status.provider_id, status.state, status.albums, status.artists, status.songs });
        auto response = rstd_co_try(co_await ensure_entities(provider_status_response(
            rstd::move(snapshots), rstd::move(libraries), statuses.as_slice())));
        response.o_payload->as_provider_status_msg()->f_statuses.retain([&](const auto& status) {
            if (status->f_id.is_none()) return false;
            auto check = checks.get((*status->f_id)->f_value);
            return check.is_some() && (*check)->epoch == status->f_epoch &&
                   provider_runtime_.get()->status_is_current(**check);
        });
        co_return Ok(rstd::move(response));
    }
    auto initial_messages() -> coro<Result<Vec<QcmMessage>, ProcessError>> {
        Vec<QcmMessage> messages;
        if (auto meta = provider_meta_status(); meta.is_some()) messages.push(rstd::move(*meta));
        messages.push(rstd_co_try(co_await provider_status()));
        co_return Ok(rstd::move(messages));
    }
    auto broadcast_status() -> coro<empty> {
        auto messages = co_await initial_messages();
        if (messages.is_err()) {
            rstd::log::error("provider status failed: {}", messages.as_ref().unwrap_err());
            co_return empty {};
        }
        for (auto& message : *messages) {
            auto sent = co_await broadcast(rstd::move(message));
            if (sent.is_err())
                rstd::log::error("provider status send failed: {}", sent.as_ref().unwrap_err());
        }
        co_return empty {};
    }
    auto process_backend_event(Arc<QcmService> service, BackendEvent event) -> coro<empty> {
        co_await broadcast_status();
        RSTD_MATCH(event) {
            RSTD_CASE(NewProvider, id) {
                auto task = start_sync_task(service.clone(), id);
                if (task.is_err())
                    rstd::log::error("provider {} initial sync failed to start: {}",
                                     id,
                                     task.as_ref().unwrap_err());
            }
            RSTD_CASE(UpdateProvider, id) {}
            RSTD_CASE(DeleteProvider, id) {}
            RSTD_CASE(ReplaceProvider, id) {}
        }
        co_return empty {};
    }
    auto process_request(Arc<QcmService> service, QcmMessage request,
                         const ProtocolSession& protocol, ResponseSink sink)
        -> coro<ProcessResult<Option<QcmMessage>>>;

public:
    auto ensure_entities(QcmMessage response, Option<Arc<QcmMessage>> request = None())
        -> coro<Result<QcmMessage, ProcessError>> {
        return entity_queries_.populate(rstd::move(response), rstd::move(request));
    }
};

} // namespace qcm::backend
