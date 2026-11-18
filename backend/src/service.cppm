export module qcm.backend:service;
import :api.entity;
import :api.provider;
import :db.sync.run;
import :entity;
import :error;
import :event;
import :model.item;
import :playback;
import :plugin.state;
import :protocol.session;
import :provider.interface;
import :provider.item;
import :provider.registry;
import :provider.runtime;
import :resource.file_delivery;
import :resource.object_fetch;
import :resource.service;
import :sync_events;
import :task;
import qcm.backend.player.integration;
import lihttpto;
import rstd;
import sqlito;
template<class T>
using AsyncMutex = rstd::async::Mutex<T>;
using alloc::rc::Rc;
using rstd::collections::HashMap;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using namespace rstd::prelude;
using rstd::async::coro;
using rstd::async::RuntimeHandle;

namespace qcm::backend
{
class MixRefresh;
class PlayerSession;
class RadioSessions;
} // namespace qcm::backend

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
    Arc<RadioSessions>                         radio_sessions_;
    Option<String>                             cache_dir_;
    Option<Arc<resource_cache::ResourceCache>> resource_store_;
    ResourceObjectFetch                        object_fetch_;
    Rc<ProviderRuntime>                        provider_runtime_;
    Weak<QcmService>                           self_ { Weak<QcmService>::make() };
    PlaybackSessions                           playback_;
    Option<Arc<PlayerSession>>                 player_;
    Option<PathBuf>                            player_preferences_;
    Box<MixRefresh>                            mix_refresh_;
    Option<EventSink>                          event_sink_;
    SyncEvents                                 sync_events_;
    Rc<TaskOwner>                              tasks_ { TaskOwner::make() };
    HashMap<i64, Weak<AsyncMutex<empty>>>      provider_change_locks_;

public:
    QcmService(sqlito::ExecutorHandle db, ProviderRegistry providers, ProviderRuntime runtime,
               Option<sqlito::ExecutorHandle> cache_db, Option<String> cache_dir,
               Option<StateDatabase> state_db);
    ~QcmService();
    static auto
    make(sqlito::ExecutorHandle db, RuntimeHandle runtime_handle, ProviderRegistry providers = {},
         Option<sqlito::ExecutorHandle> cache_db = None(), Option<String> cache_dir = None(),
         Option<String> device_id = None(), Option<String> state_db_path = None())
        -> coro<Result<Arc<QcmService>, ProcessError>>;

    auto set_event_sink(Option<EventSink> sink) -> void;
    auto start_player(PathBuf program, String base) -> coro<empty>;
    auto attach_player_integration(Box<dyn<PlayerIntegration>> integration)
        -> coro<PlayerIntegrationResult>;
    auto stop_player() -> coro<empty>;
    auto control_player(p_qcm::p_msg::M_PlayerControlReq request) -> coro<ProcessResult<empty>>;
    auto edit_player_queue(p_qcm::p_msg::M_PlayerQueueReq request) -> coro<ProcessResult<empty>>;
    static auto populate_sync_event(Weak<QcmService> weak, QcmMessage message)
        -> coro<ProcessResult<QcmMessage>>;
    static auto refresh_sync_providers(Weak<QcmService> weak) -> coro<ProcessResult<QcmMessage>>;
    auto        start_sync_events() -> void;
    auto        sync_publisher(i64 provider_id, const ProviderHandle& expected = {})
        -> Option<SyncPublisher>;
    auto flush_sync_events() -> coro<empty>;
    auto start_resource_cache() -> coro<ProcessResult<empty>>;
    auto stop_resource_cache() -> coro<empty>;
    auto invalidate_resource_cache(i64 provider) -> coro<empty>;
    auto authorize_file_client(ref<str> key) -> Result<empty, resource_cache::Error>;
    auto file_lease_response(ref<str> method, ref<str> path, const lihttpto::Headers& headers)
        -> coro<Option<resource_cache::Response>>;
    auto temporary_provider(ref<str> key) -> TmpProviderHandle;
    auto provider_instance(i64 id) -> ProviderHandle;
    auto complete_provider_mutation(Arc<QcmService> service, ProviderCommittedMutation& mutation)
        -> coro<ProcessResult<empty>>;
    static auto run_sync_task(Arc<QcmService> service, i64 provider_id, ProviderHandle instance)
        -> coro<empty>;
    auto start_sync_task(Arc<QcmService> service, i64 provider_id) -> Result<i64, ProcessError>;
    auto cancel_sync_tasks() -> void;
    static auto drain_sync_tasks(Rc<TaskOwner> tasks, coro<empty> events_stopped) -> coro<empty>;
    auto        stop_sync_tasks() -> coro<empty>;
    auto        provider_change_lock(i64 id) -> Arc<AsyncMutex<empty>>;
    auto        begin_provider_shutdown() -> void;
    auto        stop_provider_changes() -> coro<empty>;
    auto        finish_provider_replacement(i64 changed, ProviderHandle old) -> coro<empty>;
    auto        stop_providers() -> coro<empty>;
    auto        reload_provider_async(i64 changed) -> coro<Result<empty, ProcessError>>;
    auto        owns_sync_status(i64 id, const ProviderHandle& instance, u64 lifecycle) -> bool;
    auto sync_provider_async(i64 provider_id, Option<Item> item = None(),
                             ProviderHandle expected = {}) -> coro<Result<u32, ProcessError>>;
    auto sync_item_provider_async(i64 item_id) -> coro<Result<u32, ProcessError>>;
    auto refresh_mix_async(i64 mix_id, bool force, ProviderRequestOwnerHandle owner = {})
        -> coro<Result<empty, ProcessError>>;
    auto image_resource_plan(i64 id, ref<str> item_type_name, ref<str> image_type_name,
                             const lihttpto::Headers& headers)
        -> coro<Result<Option<ProviderResourcePlan>, ProcessError>>;
    auto object_resource(ProviderResourcePlan plan, lihttpto::Headers headers)
        -> coro<Result<resource_cache::ResourceDelivery, resource_cache::Error>>;
    auto audio_resource_plan(i64 id, const lihttpto::Headers& headers)
        -> coro<Result<Option<ProviderResourcePlan>, ProcessError>>;
    auto stop_radio_service() -> coro<empty>;
    auto collect_radio_sessions() -> coro<empty>;
    auto playback() -> PlaybackSessions&;
    auto resolve_playback_async(String token, usize index, String path, String query,
                                lihttpto::Headers headers)
        -> coro<ProcessResult<Option<resource_cache::ResourceDelivery>>>;
    auto broadcast(QcmMessage message) -> coro<Result<empty, ProcessError>>;
    auto provider_meta_status() -> Option<QcmMessage>;
    auto provider_status() -> coro<Result<QcmMessage, ProcessError>>;
    auto initial_messages() -> coro<Result<Vec<QcmMessage>, ProcessError>>;
    auto broadcast_status() -> coro<empty>;
    auto process_backend_event(Arc<QcmService> service, BackendEvent event) -> coro<empty>;
    auto process_request(Arc<QcmService> service, QcmMessage request,
                         const ProtocolSession& protocol, ResponseSink sink)
        -> coro<ProcessResult<Option<QcmMessage>>>;

public:
    auto ensure_entities(QcmMessage response, Option<Arc<QcmMessage>> request = None())
        -> coro<Result<QcmMessage, ProcessError>>;
};

} // namespace qcm::backend
