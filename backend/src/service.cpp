module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import rstd;
import sqlito;
import :entity;
import :event;
import :playback;
import :player.session;
import qcm.backend.player.integration;
import :protocol;
import :sync_events;
import :task;
import lihttpto;
import :api.comments;
import :api.home;
import :api.library;
import :api.mix;
import :api.network;
import :api.provider;
import :api.provider_status;
import :api.query;
import :api.radio;
import :api.recommendations;
import :api.storage;
import :api.request_input;
import :db;
import :db.query.media;
import :db.provider.status;
import rstd.log;
import :db.mix.query;
import :db.mix.refresh;
import :error;
import :error_response;
import :identity;
import :model.library;
import :model.mix;
import :pagination;
import :plugin;
import :provider;
import :resource;
import :sync_state;
import :service;
template<class T>
using AsyncMutex = rstd::async::Mutex<T>;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::tuple;
using rstd::collections::HashMap;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::async::coro;
using rstd::async::RuntimeHandle;

namespace qcm::backend
{
QcmService::QcmService(sqlito::ExecutorHandle db, ProviderRegistry providers,
                       ProviderRuntime runtime, Option<sqlito::ExecutorHandle> cache_db,
                       Option<String> cache_dir, Option<StateDatabase> state_db)
    : db_(rstd::move(db)),
      cache_db_(rstd::move(cache_db)),
      entity_queries_(db_.clone(), coordinator_.clone()),
      state_db_(rstd::move(state_db)),
      providers_(rstd::move(providers)),
      radio_sessions_(Arc<RadioSessions>::make()),
      cache_dir_(rstd::move(cache_dir)),
      provider_runtime_(make_rc<ProviderRuntime>(rstd::move(runtime))),
      playback_(db_.clone(), provider_runtime_.clone()),
      mix_refresh_(Box<MixRefresh>::make()) {}

QcmService::~QcmService() = default;

auto QcmService::make(sqlito::ExecutorHandle db, RuntimeHandle runtime_handle,
                      ProviderRegistry providers, Option<sqlito::ExecutorHandle> cache_db,
                      Option<String> cache_dir, Option<String> device_id,
                      Option<String> state_db_path) -> coro<Result<Arc<QcmService>, ProcessError>> {
    // Cancellation drops this owner, stopping and joining its database worker.
    Option<StateDatabase>          state_db;
    Option<sqlito::ExecutorHandle> state_handle;
    Option<PathBuf>                player_preferences;
    if (state_db_path.is_some()) {
        auto path = PathBuf::from(state_db_path->as_str());
        if (auto parent = path.as_path().parent())
            player_preferences = Some(PathBuf::from(*parent).join("playback.json"_str));
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
    auto service                 = Arc<QcmService>::make(rstd::move(db),
                                                         rstd::move(providers),
                                                         rstd::move(*runtime),
                                                         rstd::move(cache_db),
                                                         rstd::move(cache_dir),
                                                         rstd::move(state_db));
    service->self_               = service.downgrade();
    service->player_preferences_ = rstd::move(player_preferences);
    service->start_sync_events();
    co_return Ok(rstd::move(service));
}

auto QcmService::set_event_sink(Option<EventSink> sink) -> void {
    sync_events_.set_event_sink(sink.is_some() ? Some(sink->clone()) : None());
    event_sink_ = rstd::move(sink);
}

auto QcmService::start_player(PathBuf program, String base) -> coro<empty> {
    auto weak   = self_.clone();
    auto events = EventSink::make(
        [weak = rstd::move(weak)](QcmMessage message) -> coro<ProcessResult<empty>> {
            if (auto service = weak.upgrade())
                co_return co_await service->broadcast(rstd::move(message));
            co_return Ok(empty {});
        });
    player_ = Some(PlayerSession::make(db_.clone(),
                                       provider_runtime_.clone(),
                                       radio_sessions_.clone(),
                                       coordinator_.clone(),
                                       rstd::move(program),
                                       rstd::move(base),
                                       rstd::move(events),
                                       player_preferences_.clone()));
    co_await (*player_)->start();
    co_return empty {};
}

auto QcmService::attach_player_integration(Box<dyn<PlayerIntegration>> integration)
    -> coro<PlayerIntegrationResult> {
    if (player_.is_none()) {
        co_await integration->shutdown();
        co_return Err("player is unavailable"_Str);
    }
    auto player = (*player_).clone();
    co_return co_await player->attach_integration(rstd::move(integration));
}

auto QcmService::stop_player() -> coro<empty> {
    if (auto player = player_.take()) co_await (*player)->shutdown();
    co_return empty {};
}

auto QcmService::control_player(p_qcm::p_msg::M_PlayerControlReq request)
    -> coro<ProcessResult<empty>> {
    if (player_.is_none()) co_return Err(ProcessError::internal("player is unavailable"_str));
    auto player = (*player_).clone();
    co_return co_await player->control(rstd::move(request));
}

auto QcmService::edit_player_queue(p_qcm::p_msg::M_PlayerQueueReq request)
    -> coro<ProcessResult<empty>> {
    if (player_.is_none()) co_return Err(ProcessError::internal("player is unavailable"_str));
    auto player = (*player_).clone();
    co_return co_await player->queue(rstd::move(request));
}

auto QcmService::populate_sync_event(Weak<QcmService> weak, QcmMessage message)
    -> coro<ProcessResult<QcmMessage>> {
    if (auto service = weak.upgrade())
        co_return co_await service->ensure_entities(rstd::move(message));
    co_return Err(ProcessError::internal("sync event service is unavailable"_str));
}

auto QcmService::refresh_sync_providers(Weak<QcmService> weak) -> coro<ProcessResult<QcmMessage>> {
    if (auto service = weak.upgrade()) co_return co_await service->provider_status();
    co_return Err(ProcessError::internal("sync event service is unavailable"_str));
}

auto QcmService::start_sync_events() -> void {
    sync_events_.start(
        [weak = self_.clone()](QcmMessage message) {
            return populate_sync_event(weak.clone(), rstd::move(message));
        },
        [weak = self_.clone()] {
            return refresh_sync_providers(weak.clone());
        });
}

auto QcmService::sync_publisher(i64 provider_id, const ProviderHandle& expected)
    -> Option<SyncPublisher> {
    return sync_events_.publisher(*provider_runtime_.get(), provider_id, expected);
}

auto QcmService::flush_sync_events() -> coro<empty> { return sync_events_.flush(); }

auto QcmService::start_resource_cache() -> coro<ProcessResult<empty>> {
    if (cache_dir_.is_none() || resource_store_.is_some()) co_return Ok(empty {});
    auto opened = rstd_co_try(
        co_await resource_cache::ResourceCache::open(PathBuf::from(cache_dir_->as_str())));
    resource_store_ = Some(rstd::move(opened));
    co_return Ok(empty {});
}

auto QcmService::stop_resource_cache() -> coro<empty> {
    if (resource_store_.is_some()) {
        if ((co_await (*resource_store_)->shutdown()).is_err())
            rstd::log::warn("resource cache shutdown failed");
        resource_store_ = None();
    }
    co_return empty {};
}

auto QcmService::invalidate_resource_cache(i64 provider) -> coro<empty> {
    if (resource_store_.is_some() && (co_await (*resource_store_)->invalidate(provider)).is_err())
        rstd::log::warn("resource cache invalidation failed for provider {}", provider);
    co_return empty {};
}

auto QcmService::authorize_file_client(ref<str> key) -> Result<empty, resource_cache::Error> {
    if (resource_store_.is_none())
        return Err(resource_cache::Error::state("resource cache is unavailable"_str));
    return (*resource_store_)->authorize_file_client(key);
}

auto QcmService::file_lease_response(ref<str> method, ref<str> path,
                                     const lihttpto::Headers& headers)
    -> coro<Option<resource_cache::Response>> {
    Option<Arc<resource_cache::ResourceCache>> cache;
    if (resource_store_.is_some()) cache = Some((*resource_store_).clone());
    return resource_cache::ResourceCache::file_lease_response(
        rstd::move(cache), method, path, headers);
}

auto QcmService::temporary_provider(ref<str> key) -> TmpProviderHandle {
    return tmp_providers_.acquire(key);
}

auto QcmService::provider_instance(i64 id) -> ProviderHandle {
    return provider_runtime_.get()->acquire(id);
}

auto QcmService::complete_provider_mutation(Arc<QcmService>            service,
                                            ProviderCommittedMutation& mutation)
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
        case ProviderMutationKind::Added: return BackendEvent::NewProvider(mutation.provider_id);
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

auto QcmService::run_sync_task(Arc<QcmService> service, i64 provider_id, ProviderHandle instance)
    -> coro<empty> {
    auto publisher = service->sync_publisher(provider_id, instance);
    auto result = co_await service->sync_provider_async(provider_id, None(), rstd::move(instance));
    if (result.is_err())
        rstd::log::error("provider {} sync failed: {}", provider_id, result.as_ref().unwrap_err());
    if (publisher.is_some()) (void)publisher->refresh_providers();
    co_return empty {};
}

auto QcmService::start_sync_task(Arc<QcmService> service, i64 provider_id)
    -> Result<i64, ProcessError> {
    if (provider_runtime_.get()->find(provider_id) == nullptr)
        return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
    auto instance = provider_runtime_.get()->acquire(provider_id);
    return tasks_.get()->start_sync(provider_id,
                                    instance.clone(),
                                    run_sync_task(service.clone(), provider_id, instance.clone()));
}

auto QcmService::cancel_sync_tasks() -> void {
    sync_events_.begin_stop();
    tasks_.get()->cancel_sync();
}

auto QcmService::drain_sync_tasks(Rc<TaskOwner> tasks, coro<empty> events_stopped) -> coro<empty> {
    co_await tasks.get()->stop_sync();
    co_await rstd::move(events_stopped);
    co_return empty {};
}

auto QcmService::stop_sync_tasks() -> coro<empty> {
    cancel_sync_tasks();
    return drain_sync_tasks(tasks_.clone(), sync_events_.stop());
}

auto QcmService::provider_change_lock(i64 id) -> Arc<AsyncMutex<empty>> {
    Vec<i64> expired;
    for (auto entry : provider_change_locks_.iter())
        if (rstd::get<1>(entry)->strong_count() == usize()) expired.push(i64(*rstd::get<0>(entry)));
    for (auto key : expired) provider_change_locks_.remove(key);
    if (auto entry = provider_change_locks_.get(id))
        if (auto live = (*entry)->upgrade()) return live;
    auto lock = Arc<AsyncMutex<empty>>::make(empty {});
    provider_change_locks_.insert(id, lock.downgrade());
    return lock;
}

auto QcmService::begin_provider_shutdown() -> void { tasks_.get()->begin_provider_shutdown(); }

auto QcmService::stop_provider_changes() -> coro<empty> {
    return tasks_.get()->stop_provider_changes();
}

auto QcmService::finish_provider_replacement(i64 changed, ProviderHandle old) -> coro<empty> {
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

auto QcmService::stop_providers() -> coro<empty> {
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

auto QcmService::reload_provider_async(i64 changed) -> coro<Result<empty, ProcessError>> {
    auto old = provider_runtime_.get()->acquire(changed);
    rstd_co_try(co_await provider_runtime_.get()->reload_one(db_.clone(), providers_, changed));
    co_await finish_provider_replacement(changed, rstd::move(old));
    co_return Ok(empty {});
}

auto QcmService::owns_sync_status(i64 id, const ProviderHandle& instance, u64 lifecycle) -> bool {
    return provider_runtime_.get()->find(id) == instance.get() &&
           instance->lifecycle_epoch() == lifecycle;
}

auto QcmService::sync_provider_async(i64 provider_id, Option<Item> item, ProviderHandle expected)
    -> coro<Result<u32, ProcessError>> {
    auto instance = expected ? rstd::move(expected) : provider_runtime_.get()->acquire(provider_id);
    if (! instance || provider_runtime_.get()->find(provider_id) != instance.get())
        co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
    auto lifecycle = instance->lifecycle_epoch();
    auto publisher = sync_publisher(provider_id, instance);
    if (publisher.is_none()) co_return Err(ProcessError::NotAuth());
    if (item.is_none()) (void)co_await publisher->send(SyncCommit::SetState(SyncState::Syncing));
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

auto QcmService::sync_item_provider_async(i64 item_id) -> coro<Result<u32, ProcessError>> {
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

auto QcmService::refresh_mix_async(i64 mix_id, bool force, ProviderRequestOwnerHandle owner)
    -> coro<Result<empty, ProcessError>> {
    co_return co_await mix_refresh_->refresh(
        db_.clone(),
        mix_id,
        force,
        [this, owner = rstd::move(owner)](Item item) mutable -> coro<Result<empty, ProcessError>> {
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

auto QcmService::image_resource_plan(i64 id, ref<str> item_type_name, ref<str> image_type_name,
                                     const lihttpto::Headers& headers)
    -> coro<Result<Option<ProviderResourcePlan>, ProcessError>> {
    auto descriptor = rstd_co_try(co_await database_run(
        db_.clone(),
        [id, item_type = String::make(item_type_name), image_type = String::make(image_type_name)](
            sqlito::Connection& db) {
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

auto QcmService::object_resource(ProviderResourcePlan plan, lihttpto::Headers headers)
    -> coro<Result<resource_cache::ResourceDelivery, resource_cache::Error>> {
    co_return co_await object_fetch_.respond(
        resource_store_.is_some() ? resource_store_->as_ptr().as_raw_ptr() : nullptr,
        rstd::move(plan),
        rstd::move(headers));
}

auto QcmService::audio_resource_plan(i64 id, const lihttpto::Headers& headers)
    -> coro<Result<Option<ProviderResourcePlan>, ProcessError>> {
    auto descriptor = rstd_co_try(co_await database_run(db_.clone(), [id](sqlito::Connection& db) {
        return audio_resource_descriptor(db, id);
    }));
    if (descriptor.is_none()) co_return Ok(None<ProviderResourcePlan>());
    auto plan = rstd_co_try(co_await provider_runtime_.get()->prepare_resource_async(
        descriptor->provider_id, Resource::Audio(rstd::move(descriptor->native_id)), headers));
    co_return Ok(Some(rstd::move(plan)));
}

auto QcmService::stop_radio_service() -> coro<empty> {
    return radio_sessions_->shutdown(*provider_runtime_.get(), coordinator_, db_.clone());
}

auto QcmService::collect_radio_sessions() -> coro<empty> {
    return radio_sessions_->collect(*provider_runtime_.get(), coordinator_, db_.clone());
}

auto QcmService::playback() -> PlaybackSessions& { return playback_; }

auto QcmService::resolve_playback_async(String token, usize index, String path, String query,
                                        lihttpto::Headers headers)
    -> coro<ProcessResult<Option<resource_cache::ResourceDelivery>>> {
    co_return co_await playback_.resolve_resource(rstd::move(token),
                                                  index,
                                                  rstd::move(path),
                                                  rstd::move(query),
                                                  rstd::move(headers),
                                                  resource_store_.clone());
}

auto QcmService::broadcast(QcmMessage message) -> coro<Result<empty, ProcessError>> {
    if (event_sink_.is_some()) co_return co_await event_sink_->send_message(rstd::move(message));
    co_return Ok(empty {});
}

auto QcmService::provider_meta_status() -> Option<QcmMessage> {
    if (providers_.metas().is_empty()) return None();
    auto rsp = Box<p_qcm::p_msg::M_ProviderMetaStatusMsg>::make();
    for (const auto& meta : providers_.metas())
        rsp->f_metas.push(rstd::into<Box<p_qcm::p_msg::p_model::M_ProviderMeta>>(
            ref<ProviderMetaInfo>::from_raw_parts(rstd::addressof(meta))));
    rsp->f_full  = true;
    auto message = qcm_message(i32(), rstd::move(rsp));
    return Some(rstd::move(message));
}

auto QcmService::provider_status() -> coro<Result<QcmMessage, ProcessError>> {
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

auto QcmService::initial_messages() -> coro<Result<Vec<QcmMessage>, ProcessError>> {
    Vec<QcmMessage> messages;
    if (auto meta = provider_meta_status(); meta.is_some()) messages.push(rstd::move(*meta));
    messages.push(rstd_co_try(co_await provider_status()));
    if (player_.is_some()) {
        messages.push(qcm_message(
            i32(), Box<p_qcm::p_msg::M_PlayerQueueMsg>::make((*player_)->queue_state())));
        messages.push(
            qcm_message(i32(), Box<p_qcm::p_msg::M_PlayerStateMsg>::make((*player_)->state())));
    }
    co_return Ok(rstd::move(messages));
}

auto QcmService::broadcast_status() -> coro<empty> {
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

auto QcmService::process_backend_event(Arc<QcmService> service, BackendEvent event) -> coro<empty> {
    co_await broadcast_status();
    RSTD_MATCH(event) {
        RSTD_CASE(NewProvider, id) {
            auto task = start_sync_task(service.clone(), id);
            if (task.is_err())
                rstd::log::error(
                    "provider {} initial sync failed to start: {}", id, task.as_ref().unwrap_err());
        }
        RSTD_CASE(UpdateProvider, id) {}
        RSTD_CASE(DeleteProvider, id) {}
        RSTD_CASE(ReplaceProvider, id) {}
    }
    co_return empty {};
}

auto QcmService::ensure_entities(QcmMessage response, Option<Arc<QcmMessage>> request)
    -> coro<Result<QcmMessage, ProcessError>> {
    return entity_queries_.populate(rstd::move(response), rstd::move(request));
}

class QcmService::RequestDispatcher {
    Arc<QcmService> service_;

public:
    explicit RequestDispatcher(Arc<QcmService> service): service_(rstd::move(service)) {}
    auto storage_info_async(i32 request_id) -> coro<Result<QcmMessage, ProcessError>> {
        resource_cache::Usage usage;
        if (service_->resource_store_.is_some()) {
            usage = rstd_co_try(co_await (*service_->resource_store_)->usage());
        }
        i64 cache_size {};
        if (service_->cache_db_.is_some())
            cache_size = rstd_co_try(
                co_await database_run(service_->cache_db_->clone(),
                                      [](sqlito::Connection& db) -> Result<i64, sqlito::Error> {
                                          return Ok(sqlite_disk_bytes(db));
                                      }));
        auto main_size = rstd_co_try(co_await database_run(
            service_->db_.clone(), [](sqlito::Connection& db) -> Result<i64, sqlito::Error> {
                return Ok(sqlite_disk_bytes(db));
            }));
        auto total     = u64((main_size + cache_size).to_primitive());
        total          = usage.metadata > u64::MAX - total ? u64::MAX : total + usage.metadata;
        co_return Ok(storage_info_response(i64(total.to_primitive()),
                                           i64(usage.audio.to_primitive()),
                                           i64(usage.image.to_primitive()),
                                           request_id));
    }
    auto authenticate_temporary(const QcmMessage& request)
        -> coro<Result<QcmMessage, ProcessError>> {
        co_return co_await auth_provider_async(
            service_->tmp_providers_, request.f_id, *request.o_payload->as_auth_provider_req());
    }
    auto temporary_qr(const QcmMessage& request) -> coro<Result<QcmMessage, ProcessError>> {
        co_return co_await qr_auth_url_async(
            service_->tmp_providers_, request.f_id, *request.o_payload->as_qr_auth_url_req());
    }
    auto mutate_provider(const QcmMessage& request, ProviderMutationWrite& write)
        -> coro<ProcessResult<ProviderMutationOutcome>> {
        RSTD_MATCH(*request.o_payload) {
            RSTD_CASE_PAYLOAD(add_provider_req, entry) {
                service_->coordinator_.bump();
                co_return co_await add_provider_async(service_->tmp_providers_,
                                                      service_->providers_,
                                                      service_->db_.clone(),
                                                      *entry,
                                                      write);
            }
            RSTD_CASE_PAYLOAD(update_provider_req, entry) {
                auto update = rstd_co_try(provider_update_request(*entry));
                co_return co_await update_provider_async(service_->tmp_providers_,
                                                         *service_->provider_runtime_.get(),
                                                         service_->coordinator_,
                                                         service_->providers_,
                                                         service_->db_.clone(),
                                                         rstd::move(update),
                                                         write);
            }
            RSTD_CASE_PAYLOAD(replace_provider_req, entry) {
                co_return co_await replace_provider_async(service_->tmp_providers_,
                                                          *service_->provider_runtime_.get(),
                                                          service_->coordinator_,
                                                          service_->providers_,
                                                          service_->db_.clone(),
                                                          *entry,
                                                          write);
            }
            RSTD_CASE_PAYLOAD(delete_provider_req, entry) {
                co_return co_await delete_provider_async(
                    service_->db_.clone(), service_->coordinator_, *entry, write);
            }
            RSTD_CASE_DEFAULT {
                co_return Err(ProcessError::UnsupportedMessageType(request.f_type));
            }
        }
    }
    auto execute_provider_action(Arc<QcmService> service, Arc<QcmMessage> input,
                                 ProviderAction action, ProviderCancelHandle cancellable)
        -> coro<ProcessResult<QcmMessage>> {
        const auto&               request = *input;
        ProcessResult<QcmMessage> response =
            Err(ProcessError::UnsupportedMessageType(request.f_type));
        switch (action.tag) {
        case QcmPayload::Tag::auth_provider_req:
            response = co_await authenticate_temporary(request);
            break;
        case QcmPayload::Tag::qr_auth_url_req: response = co_await temporary_qr(request); break;
        case QcmPayload::Tag::delete_tmp_provider_req:
            response = delete_tmp_provider(service_->tmp_providers_,
                                           request.f_id,
                                           *request.o_payload->as_delete_tmp_provider_req());
            break;
        default: {
            ProviderMutationWrite write(rstd::move(cancellable));
            auto                  outcome = co_await mutate_provider(request, write);
            if (outcome.is_err()) {
                if (action.persisted()) co_await service_->broadcast_status();
                co_return Err(rstd::move(outcome).unwrap_err());
            }
            if (outcome->is_Committed()) {
                rstd_co_try(co_await service_->complete_provider_mutation(
                    service.clone(), outcome->as_Committed().mutation));
            } else if (action.persisted()) {
                co_await service_->broadcast_status();
            }
            response = Ok(provider_mutation_response(request.f_id, rstd::move(*outcome)));
            break;
        }
        }
        co_return co_await service_->ensure_entities(rstd_co_try(rstd::move(response)),
                                                     Some(input.clone()));
    }
    static auto run_item_sync_task(Arc<QcmService> service, Item item, ProviderHandle instance,
                                   ResponseSink sink, i32 request_id) -> coro<empty> {
        auto provider_id = item.provider_id;
        auto publisher   = service->sync_publisher(provider_id, instance);
        auto result      = co_await service->sync_provider_async(
            provider_id, Some(rstd::move(item)), rstd::move(instance));
        if (publisher.is_some()) (void)publisher->refresh_providers();
        Result<QcmMessage, ProcessError> response =
            result.is_ok() ? co_await service->ensure_entities(ok_response(request_id))
                           : Result<QcmMessage, ProcessError>(Err(rstd::move(result).unwrap_err()));
        (void)co_await sink.respond(request_id, rstd::move(response));
        co_return empty {};
    }
    auto start_item_sync_task(Arc<QcmService> service, i64 item_id, i32 request_id,
                              ResponseSink sink) -> coro<Result<empty, ProcessError>> {
        auto item = rstd_co_try(
            co_await database_run(service_->db_.clone(),
                                  [item_id](sqlito::Connection& db) -> ProcessResult<Option<Item>> {
                                      return Ok(rstd_try(read_item_by_id(db, item_id)));
                                  }));
        if (item.is_none()) co_return Err(ProcessError::NotFound());
        auto provider_id = item->provider_id;
        auto instance    = service_->provider_runtime_.get()->acquire(provider_id);
        if (! instance)
            co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
        rstd_co_try(service_->tasks_.get()->start_item_sync(
            provider_id,
            instance.clone(),
            request_id,
            rstd::move(sink),
            [service  = service.clone(),
             item     = rstd::move(*item),
             instance = instance.clone(),
             request_id](ResponseSink response) mutable {
                return run_item_sync_task(service.clone(),
                                          rstd::move(item),
                                          instance.clone(),
                                          rstd::move(response),
                                          request_id);
            }));
        co_return Ok(empty {});
    }
    static auto run_provider_request_task(Arc<QcmService> service, Arc<QcmMessage> input,
                                          ProviderRequestOwnerHandle owner, ResponseSink sink)
        -> coro<empty> {
        RequestDispatcher dispatcher(service.clone());
        auto              response =
            co_await dispatcher.execute_provider_request_async(input.clone(), rstd::move(owner));
        (void)co_await sink.respond(input->f_id, rstd::move(response));
        co_return empty {};
    }
    auto start_provider_request_task(Arc<QcmService> service, Arc<QcmMessage> input,
                                     ResponseSink sink) -> Result<empty, ProcessError> {
        auto request_id = input->f_id;
        rstd_try(service_->tasks_.get()->start_provider_request(
            request_id,
            rstd::move(sink),
            [service = service.clone(), input = rstd::move(input)](ProviderRequestOwnerHandle owner,
                                                                   ResponseSink response) mutable {
                return run_provider_request_task(
                    service.clone(), rstd::move(input), rstd::move(owner), rstd::move(response));
            }));
        return Ok(empty {});
    }
    auto start_provider_change(Arc<QcmService> service, RequestInput request, ResponseSink sink)
        -> bool {
        auto input      = request.message();
        auto action     = rstd::move(request).into_action();
        auto request_id = input->f_id;
        return service_->tasks_.get()
            ->start_provider_change(
                request_id,
                rstd::move(sink),
                [service = service.clone(), input = rstd::move(input), action = rstd::move(action)](
                    ProviderCancelHandle cancellable, ResponseSink response) mutable {
                    return run_provider_change(service.clone(),
                                               rstd::move(input),
                                               rstd::move(response),
                                               rstd::move(cancellable),
                                               rstd::move(action));
                })
            .is_ok();
    }
    auto execute_provider_request_async(Arc<QcmMessage> input, ProviderRequestOwnerHandle owner)
        -> coro<Result<QcmMessage, ProcessError>> {
        const auto& request = *input;
        if (request.o_payload.is_none())
            co_return Err(ProcessError::UnsupportedMessageType(request.f_type));
        RSTD_MATCH(*request.o_payload) {
            RSTD_CASE_PAYLOAD(get_home_recommendations_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(
                        co_await get_home_recommendations_async(*service_->provider_runtime_.get(),
                                                                service_->db_.clone(),
                                                                service_->coordinator_,
                                                                request.f_id,
                                                                *payload,
                                                                owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_radio_queues_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await get_radio_queues_async(*service_->provider_runtime_.get(),
                                                                service_->db_.clone(),
                                                                service_->coordinator_,
                                                                request.f_id,
                                                                *payload,
                                                                owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(open_queue_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await open_queue_async(*service_->provider_runtime_.get(),
                                                          *service_->radio_sessions_,
                                                          service_->coordinator_,
                                                          service_->db_.clone(),
                                                          request.f_id,
                                                          *payload,
                                                          owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_radio_preview_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await get_radio_preview_async(*service_->provider_runtime_.get(),
                                                                 *service_->radio_sessions_,
                                                                 service_->coordinator_,
                                                                 service_->db_.clone(),
                                                                 request.f_id,
                                                                 *payload,
                                                                 owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_queue_next_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await next_queue_async(*service_->provider_runtime_.get(),
                                                          *service_->radio_sessions_,
                                                          service_->coordinator_,
                                                          service_->db_.clone(),
                                                          request.f_id,
                                                          *payload,
                                                          owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(close_queue_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await close_queue_async(*service_->provider_runtime_.get(),
                                                           *service_->radio_sessions_,
                                                           service_->coordinator_,
                                                           service_->db_.clone(),
                                                           request.f_id,
                                                           *payload,
                                                           owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_home_blocks_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await get_home_blocks_async(*service_->provider_runtime_.get(),
                                                               service_->db_.clone(),
                                                               service_->coordinator_,
                                                               request.f_id,
                                                               *payload,
                                                               owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_home_block_items_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(
                        co_await get_home_block_items_async(*service_->provider_runtime_.get(),
                                                            service_->db_.clone(),
                                                            service_->coordinator_,
                                                            request.f_id,
                                                            *payload,
                                                            owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_comments_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await comments_async(*service_->provider_runtime_.get(),
                                                        service_->db_.clone(),
                                                        service_->coordinator_,
                                                        request.f_id,
                                                        *payload,
                                                        owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_comment_replies_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await comment_replies_async(*service_->provider_runtime_.get(),
                                                               service_->db_.clone(),
                                                               service_->coordinator_,
                                                               request.f_id,
                                                               *payload,
                                                               owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(set_comment_like_req, payload) {
                co_return co_await service_->ensure_entities(
                    rstd_co_try(co_await set_comment_like_async(*service_->provider_runtime_.get(),
                                                                service_->db_.clone(),
                                                                service_->coordinator_,
                                                                request.f_id,
                                                                *payload,
                                                                owner)),
                    Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_subtitle_req, payload) {
                const auto& entry        = *payload;
                auto        plan         = rstd_co_try(co_await prepare_subtitle(
                    *service_->provider_runtime_.get(), service_->db_.clone(), entry));
                owner.get()->provider_id = plan.target.provider_id;
                owner.get()->instance    = plan.instance.clone();
                auto response = co_await execute_subtitle(*service_->provider_runtime_.get(),
                                                          service_->db_.clone(),
                                                          request.f_id,
                                                          rstd::move(plan));
                owner.get()->instance.reset();
                co_return co_await service_->ensure_entities(rstd_co_try(rstd::move(response)),
                                                             Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(set_favorite_req, payload) {
                const auto& entry        = *payload;
                auto        plan         = rstd_co_try(co_await prepare_favorite(
                    *service_->provider_runtime_.get(), service_->db_.clone(), entry));
                owner.get()->provider_id = plan.target.provider_id;
                owner.get()->instance    = plan.instance.clone();
                auto response = co_await execute_favorite(*service_->provider_runtime_.get(),
                                                          service_->coordinator_,
                                                          service_->db_.clone(),
                                                          request.f_id,
                                                          entry,
                                                          rstd::move(plan));
                owner.get()->instance.reset();
                co_return co_await service_->ensure_entities(rstd_co_try(rstd::move(response)),
                                                             Some(input.clone()));
            }
            RSTD_CASE_PAYLOAD(get_mix_songs_req, payload) {
                const auto& entry = *payload;
                auto        query = rstd_co_try(prepare_mix_songs(entry));
                rstd_co_try(co_await service_->refresh_mix_async(
                    query.mix_id, entry.f_force_refresh, owner));
                auto response = rstd_co_try(co_await query_with_entities(
                    request.f_id, [request_id = request.f_id, query](auto& db) {
                        return mix_songs(db, request_id, query);
                    }));
                if (response.is_none())
                    co_return Err(ProcessError::internal("mix query produced no response"_str));
                co_return Ok(rstd::move(*response));
            }
            RSTD_CASE_PAYLOAD(get_song_ids_req, payload) {
                auto query = rstd_co_try(query_detail::prepare_song_query(*payload));
                if (query.is_Songs())
                    for (const auto& filter : query.as_Songs().query.filters.values)
                        if (filter.value.is_MixId())
                            rstd_co_try(co_await service_->refresh_mix_async(
                                filter.value.as_MixId().id, false, owner));
                auto response = rstd_co_try(co_await query_with_entities(
                    request.f_id, [request_id = request.f_id, query = rstd::move(query)](auto& db) {
                        return query_get_song_ids_prepared(db, request_id, query);
                    }));
                if (response.is_none())
                    co_return Err(ProcessError::internal("song query produced no response"_str));
                co_return Ok(rstd::move(*response));
            }
            RSTD_CASE_DEFAULT { break; }
        }
        co_return Err(ProcessError::UnsupportedMessageType(request.f_type));
    }
    auto process_request(Arc<QcmService> service, QcmMessage request,
                         const ProtocolSession& protocol, ResponseSink sink)
        -> coro<ProcessResult<Option<QcmMessage>>> {
        rstd_co_try(protocol.check(request));
        auto prepared = rstd_co_try(RequestInput::prepare(rstd::move(request)));
        auto input    = prepared.message();
        switch (prepared.kind()) {
        case RequestKind::ItemSync: {
            rstd_co_try(co_await start_item_sync_task(service.clone(),
                                                      input->o_payload->as_sync_item_req()->f_id,
                                                      input->f_id,
                                                      sink.clone()));
            co_return Ok(None<QcmMessage>());
        }
        case RequestKind::ProviderRequest: {
            rstd_co_try(start_provider_request_task(service.clone(), input.clone(), sink.clone()));
            co_return Ok(None<QcmMessage>());
        }
        case RequestKind::ProviderChange: {
            if (! start_provider_change(service.clone(), rstd::move(prepared), sink.clone()))
                co_return Err(ProcessError::internal("provider service is stopping"_str));
            co_return Ok(None<QcmMessage>());
        }
        case RequestKind::Direct: break;
        }
        auto dispatched = rstd_co_try(co_await dispatch_async(service.clone(), input.clone()));
        if (dispatched.is_none()) co_return Ok(None<QcmMessage>());
        co_return Ok(Some(rstd_co_try(
            co_await service_->ensure_entities(rstd::move(*dispatched), Some(input.clone())))));
    }
    auto dispatch_async(Arc<QcmService> service, Arc<QcmMessage> input)
        -> coro<Result<Option<QcmMessage>, ProcessError>> {
        const auto& request = *input;
        auto        type    = request.f_type;
        if (request.o_payload->is_test_req()) {
            auto rsp         = Box<p_qcm::p_msg::M_TestRsp>::make();
            rsp->f_test_data = String::make("Echo: "_str);
            rsp->f_test_data.push_str(request.o_payload->as_test_req()->f_test_data.as_str());
            if (request.o_payload->as_test_req()->f_ui_client) rsp->f_ui_accepted = Some(true);
            auto response = qcm_message(request.f_id, rstd::move(rsp));
            co_return co_await with_entities(request.f_id, rstd::move(response));
        }
        RSTD_MATCH(*request.o_payload) {
            RSTD_CASE_PAYLOAD(player_control_req, entry) {
                p_qcm::p_msg::M_PlayerControlReq command;
                command.f_action                = entry->f_action;
                command.f_position_ms           = entry->f_position_ms;
                command.f_volume                = entry->f_volume;
                command.f_fade_ms               = entry->f_fade_ms;
                command.f_source                = entry->f_source.clone();
                command.f_expected_source_epoch = entry->f_expected_source_epoch;
                rstd_co_try(co_await service_->control_player(rstd::move(command)));
                co_return Ok(Some(ok_response(request.f_id)));
            }
            RSTD_CASE_PAYLOAD(player_queue_req, entry) {
                p_qcm::p_msg::M_PlayerQueueReq command;
                command.f_action      = entry->f_action;
                command.f_song_ids    = entry->f_song_ids.clone();
                command.f_index       = entry->f_index;
                command.f_count       = entry->f_count;
                command.f_destination = entry->f_destination;
                command.f_loop_mode   = entry->f_loop_mode;
                command.f_shuffle     = entry->f_shuffle;
                command.f_play        = entry->f_play;
                command.f_name        = entry->f_name.clone();
                if (entry->f_radio_queue_id.is_some()) {
                    auto id                  = Box<p_qcm::p_msg::p_id::M_RadioQueueId>::make();
                    id->f_value              = (*entry->f_radio_queue_id)->f_value;
                    command.f_radio_queue_id = Some(rstd::move(id));
                }
                command.f_configuration     = entry->f_configuration.clone();
                command.f_expected_revision = entry->f_expected_revision;
                command.f_random_mode       = entry->f_random_mode;
                command.f_radio_reason      = entry->f_radio_reason;
                rstd_co_try(co_await service_->edit_player_queue(rstd::move(command)));
                co_return Ok(Some(ok_response(request.f_id)));
            }
            RSTD_CASE(playlog_req) {
                co_return optional_response(co_await database_run(
                    service_->db_.clone(),
                    [input       = input.clone(),
                     playing     = service_->provider_runtime_->playing_state(),
                     coordinator = service_->coordinator_.clone()](sqlito::Connection& db) mutable {
                        return playlog(
                            db, coordinator, *playing, *input, *input->o_payload->as_playlog_req());
                    }));
            }
            RSTD_CASE(get_mixs_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return mixs(db, *input, *input->o_payload->as_get_mixs_req());
                    });
            }
            RSTD_CASE(get_mix_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return mix(db, *input, *input->o_payload->as_get_mix_req());
                    });
            }
            RSTD_CASE(get_songs_by_id_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return songs_by_id(db, *input, *input->o_payload->as_get_songs_by_id_req());
                    });
            }
            RSTD_CASE(get_album_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return album(db, *input, *input->o_payload->as_get_album_req());
                    });
            }
            RSTD_CASE(get_artist_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return artist(db, *input, *input->o_payload->as_get_artist_req());
                    });
            }
            RSTD_CASE(get_albums_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_get_albums(
                            db, input->f_id, *input->o_payload->as_get_albums_req());
                    });
            }
            RSTD_CASE(get_artists_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_get_artists(
                            db, input->f_id, *input->o_payload->as_get_artists_req());
                    });
            }
            RSTD_CASE(get_album_artists_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_get_album_artists(
                            db, input->f_id, *input->o_payload->as_get_album_artists_req());
                    });
            }
            RSTD_CASE(get_artist_album_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_get_artist_album(
                            db, input->f_id, *input->o_payload->as_get_artist_album_req());
                    });
            }
            RSTD_CASE(get_remote_mixs_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_get_remote_mixs(
                            db, input->f_id, *input->o_payload->as_get_remote_mixs_req());
                    });
            }
            RSTD_CASE(search_req) {
                co_return co_await query_with_entities(
                    request.f_id, [input = input.clone()](auto& db) {
                        return query_search(db, input->f_id, *input->o_payload->as_search_req());
                    });
            }
            RSTD_CASE(create_mix_req) {
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto& coordinator, const QcmMessage& request) {
                        return create_mix(
                            db, coordinator, request.f_id, *request.o_payload->as_create_mix_req());
                    }));
            }
            RSTD_CASE(mix_manipulate_req) {
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto& coordinator, const QcmMessage& request) {
                        return manipulate_mix(db,
                                              coordinator,
                                              request.f_id,
                                              *request.o_payload->as_mix_manipulate_req());
                    }));
            }
            RSTD_CASE(link_mix_req) {
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto& coordinator, const QcmMessage& request) {
                        return link_mix(
                            db, coordinator, request.f_id, *request.o_payload->as_link_mix_req());
                    }));
            }
            RSTD_CASE(delete_mix_req) {
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto& coordinator, const QcmMessage& request) {
                        return delete_mix(
                            db, coordinator, request.f_id, *request.o_payload->as_delete_mix_req());
                    }));
            }
            RSTD_CASE(get_network_settings_req) {
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto&, const QcmMessage& request) {
                        return get_network_settings(db, request.f_id);
                    }));
            }
            RSTD_CASE(update_network_settings_req) {
                service_->coordinator_.bump();
                co_return optional_response(co_await database_request(
                    input.clone(), [](auto& db, auto&, const QcmMessage& request) {
                        return update_network_settings(
                            db, request.f_id, *request.o_payload->as_update_network_settings_req());
                    }));
            }
            RSTD_CASE(get_provider_metas_req) {
                co_return optional_response(get_provider_metas(service_->providers_, request.f_id));
            }
            RSTD_CASE_PAYLOAD(create_tmp_provider_req, payload) {
                co_return optional_response(create_tmp_provider(service_->tmp_providers_,
                                                                service_->providers_,
                                                                *service_->provider_runtime_.get(),
                                                                request.f_id,
                                                                *payload));
            }
            RSTD_CASE_PAYLOAD(get_provider_settings_req, payload) {
                co_return optional_response(get_provider_settings(
                    *service_->provider_runtime_.get(), request.f_id, *payload));
            }
            RSTD_CASE_PAYLOAD(get_share_link_req, payload) {
                co_return optional_response(
                    co_await get_share_link_async(*service_->provider_runtime_.get(),
                                                  service_->db_.clone(),
                                                  request.f_id,
                                                  *payload));
            }
            RSTD_CASE_PAYLOAD(sync_req, payload) {
                auto& entry      = *payload;
                auto provider_id = rstd_co_try(required_id(entry.f_provider_id, "provider_id"_str));
                auto handle = rstd_co_try(service_->start_sync_task(service.clone(), provider_id));
                auto rsp    = Box<p_qcm::p_msg::M_SyncRsp>::make();
                rsp->f_handle = handle;
                auto response = qcm_message(request.f_id, rstd::move(rsp));
                co_return co_await with_entities(request.f_id, rstd::move(response));
            }
            RSTD_CASE(get_storage_info_req) {
                co_return optional_response(co_await storage_info_async(request.f_id));
            }
            RSTD_CASE_DEFAULT { break; }
        }
        co_return Err(ProcessError::UnsupportedMessageType(type));
    }
    static auto optional_response(QcmMessage response) -> Result<Option<QcmMessage>, ProcessError> {
        return Ok(Some(rstd::move(response)));
    }
    static auto optional_response(Result<QcmMessage, ProcessError> response)
        -> Result<Option<QcmMessage>, ProcessError> {
        return Ok(Some(rstd_try(rstd::move(response))));
    }
    template<class F>
    auto database_request(Arc<QcmMessage> input, F function)
        -> coro<Result<QcmMessage, ProcessError>> {
        co_return co_await database_run(
            service_->db_.clone(),
            [input       = rstd::move(input),
             coordinator = service_->coordinator_.clone(),
             function    = rstd::move(function)](sqlito::Connection& db) mutable {
                return function(db, coordinator, *input);
            });
    }
    auto with_entities(i32, QcmMessage response) -> coro<Result<Option<QcmMessage>, ProcessError>> {
        co_return optional_response(co_await service_->ensure_entities(rstd::move(response)));
    }
    template<class F>
    auto query_with_entities(i32, F query) -> coro<Result<Option<QcmMessage>, ProcessError>> {
        return service_->entity_queries_.query(rstd::move(query));
    }

    static auto execute_with_temporary(Arc<QcmService> service, Arc<QcmMessage> input,
                                       ResponseSink sink, ProviderAction action,
                                       ProviderCancelHandle cancellable) -> coro<empty> {
        RequestDispatcher dispatcher(service.clone());
        auto              tmp =
            action.temporary.is_empty() || action.tag == QcmPayload::Tag::delete_tmp_provider_req
                ? TmpProviderHandle {}
                : service->temporary_provider(action.temporary.as_str());
        ProcessResult<QcmMessage> response =
            Err(ProcessError::UnsupportedMessageType(input->f_type));
        if (tmp) {
            auto guard = co_await tmp.get()->gate->lock();
            response   = co_await dispatcher.execute_provider_action(
                service.clone(), input.clone(), rstd::move(action), cancellable);
        } else {
            response = co_await dispatcher.execute_provider_action(
                service.clone(), input.clone(), rstd::move(action), cancellable);
        }
        (void)co_await sink.respond(input->f_id, rstd::move(response));
        co_return empty {};
    }

    static auto run_provider_change(Arc<QcmService> service, Arc<QcmMessage> input,
                                    ResponseSink sink, ProviderCancelHandle cancellable,
                                    ProviderAction action) -> coro<empty> {
        if (action.persisted()) {
            auto lock  = service->provider_change_lock(action.provider_id.unwrap_or(i64()));
            auto guard = co_await lock->lock();
            co_await execute_with_temporary(
                service.clone(), input.clone(), sink.clone(), rstd::move(action), cancellable);
        } else {
            co_await execute_with_temporary(
                service.clone(), input.clone(), sink.clone(), rstd::move(action), cancellable);
        }
        co_return empty {};
    }
};

auto QcmService::process_request(Arc<QcmService> service, QcmMessage request,
                                 const ProtocolSession& protocol, ResponseSink sink)
    -> coro<ProcessResult<Option<QcmMessage>>> {
    RequestDispatcher dispatcher(service.clone());
    co_return co_await dispatcher.process_request(
        service.clone(), rstd::move(request), protocol, rstd::move(sink));
}

} // namespace qcm::backend
