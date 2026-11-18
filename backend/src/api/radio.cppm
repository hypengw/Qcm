export module qcm.backend:api.radio;
import :protocol.message;
import :provider.interface;
import :provider.item;
import :provider.queue;
import :provider.runtime;
import :db.sync.retention;
import :db.sync.run;
import :error;
import :radio.error;
import rstd;
import sqlito;
template<class T>
using AsyncMutex = rstd::async::Mutex<T>;
using alloc::rc::Rc;
using rstd::async::JoinHandle;
namespace radio_watch = rstd::async::watch;
using rstd::collections::HashMap;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct RadioPage {
    String             session_id;
    u64                version {};
    Vec<i64>           song_ids;
    QueueConfiguration configuration;
    QueuePresentation  presentation;
    bool               exhausted {};
    auto               clone() const -> RadioPage {
        return { session_id.clone(),   version,  song_ids.clone(), configuration.clone(),
                 presentation.clone(), exhausted };
    }
};
using RadioResult = Result<RadioPage, RadioError>;
struct RadioOperation {
    String              fingerprint;
    ProviderHandle      instance;
    u64                 epoch {};
    i64                 created {};
    AsyncMutex<empty>   mutex { empty {} };
    Option<RadioResult> result;
    RadioOperation(String id, ProviderHandle provider, u64 generation, i64 time)
        : fingerprint(rstd::move(id)),
          instance(rstd::move(provider)),
          epoch(generation),
          created(time) {}
};
enum class RadioSessionStatus
{
    Pending,
    Active,
    Closing,
    Released
};
struct RadioSession {
    String                      id;
    i64                         queue_id {};
    String                      native_id;
    i64                         provider_id {};
    String                      state;
    QueueConfiguration          configuration;
    QueuePresentation           presentation;
    bool                        exhausted {};
    u64                         version { 1 };
    i64                         touched {};
    Option<ItemRetentionHandle> retention;
    ProviderHandle              instance;
    u64                         epoch {};
    ProviderSyncConfig          sync;
    AsyncMutex<empty>           gate { empty {} };
    RadioSessionStatus          status { RadioSessionStatus::Pending };
    Option<JoinHandle<empty>>   closing;
    radio_watch::Sender<bool>   released { false };
};
using RadioSessionHandle = Rc<RadioSession>;
struct RadioPreparation {
    AsyncMutex<empty>  gate { empty {} };
    Option<RadioPage>  page;
    QueueConfiguration requested;
};
class RadioSessions {
    enum class Cleanup
    {
        Expired,
        Provider,
        All
    };
    Mutex<HashMap<String, RadioSessionHandle>>  sessions_ { HashMap<String, RadioSessionHandle>() };
    Mutex<HashMap<String, Arc<RadioOperation>>> operations_ {
        HashMap<String, Arc<RadioOperation>>()
    };
    Mutex<HashMap<i64, Rc<RadioPreparation>>> preparations_ {
        HashMap<i64, Rc<RadioPreparation>>()
    };
    bool        stopped_ {};
    static auto release(ProviderRuntime&, SyncCoordinator, sqlito::ExecutorHandle,
                        RadioSessionHandle) -> rstd::async::coro<empty>;
    auto        release_selected(ProviderRuntime&, SyncCoordinator&, sqlito::ExecutorHandle,
                                 ProviderHandle, Cleanup) -> rstd::async::coro<empty>;

public:
    auto is_stopped() const -> bool;
    auto preparation(i64 queue_id) -> Result<Rc<RadioPreparation>, RadioError>;
    auto operation(ref<str> key, ref<str> fingerprint, ProviderHandle instance, u64 epoch)
        -> Result<Arc<RadioOperation>, RadioError>;
    auto adopt(RadioSession session) -> RadioSessionHandle;
    auto publish(RadioSessionHandle owner) -> bool;
    auto snapshot(ref<str> id) -> RadioSessionHandle;
    auto peek(ref<str> id) -> RadioSessionHandle;
    auto start_close(ProviderRuntime& runtime, SyncCoordinator& coordinator,
                     sqlito::ExecutorHandle db, RadioSessionHandle session) -> void;
    auto close(ProviderRuntime&, SyncCoordinator&, sqlito::ExecutorHandle, String)
        -> rstd::async::coro<empty>;
    auto collect(ProviderRuntime& runtime, SyncCoordinator& coordinator, sqlito::ExecutorHandle db)
        -> rstd::async::coro<empty>;
    auto invalidate(ProviderRuntime& runtime, SyncCoordinator& coordinator,
                    sqlito::ExecutorHandle db, ProviderHandle provider) -> rstd::async::coro<empty>;
    auto shutdown(ProviderRuntime& runtime, SyncCoordinator& coordinator, sqlito::ExecutorHandle db)
        -> rstd::async::coro<empty>;
};

auto radio_now() -> i64;
auto queue_session_message(const RadioPage& page) -> Box<p_qcm::p_msg::p_model::M_QueueSession>;
auto wait_radio_session_closed(RadioSessionHandle session) -> rstd::async::coro<empty>;
auto close_queue_async(ProviderRuntime& runtime, RadioSessions& sessions,
                       SyncCoordinator& coordinator, sqlito::ExecutorHandle db, i32 request_id,
                       const p_qcm::p_msg::M_CloseQueueReq& entry,
                       ProviderRequestOwnerHandle           owner = {})
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>>;
auto open_queue_async(ProviderRuntime& runtime, RadioSessions& sessions,
                      SyncCoordinator& coordinator, sqlito::ExecutorHandle db, i32 request_id,
                      const p_qcm::p_msg::M_OpenQueueReq& entry,
                      ProviderRequestOwnerHandle          owner = {})
    -> rstd::async::coro<ProcessResult<p_qcm::p_msg::M_QcmMessage>>;
auto get_radio_preview_async(ProviderRuntime& runtime, RadioSessions& sessions,
                             SyncCoordinator& coordinator, sqlito::ExecutorHandle db,
                             i32 request_id, const p_qcm::p_msg::M_GetRadioPreviewReq& entry,
                             ProviderRequestOwnerHandle owner = {})
    -> rstd::async::coro<ProcessResult<p_qcm::p_msg::M_QcmMessage>>;
auto activate_radio_page_async(ProviderRuntime& runtime, RadioSessions& sessions,
                               SyncCoordinator& coordinator, sqlito::ExecutorHandle db,
                               const p_qcm::p_msg::M_OpenQueueReq& entry)
    -> rstd::async::coro<ProcessResult<RadioPage>>;
auto next_queue_async(ProviderRuntime& runtime, RadioSessions& sessions,
                      SyncCoordinator& coordinator, sqlito::ExecutorHandle db, i32 request_id,
                      const p_qcm::p_msg::M_GetQueueNextReq& entry,
                      ProviderRequestOwnerHandle             owner = {})
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>>;
} // namespace qcm::backend
