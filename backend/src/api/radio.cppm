module;
#include <rstd/macro.hpp>
export module qcm.backend:api.radio;
export import :provider.runtime;
import :protocol.message;
export import :error;
import :db.sync.retention;
import :db.query.radio;
import :db.database;
import :db.timestamp;
import :db.sync_batch.item_reference;
import :db.error.process;
import :db.error.provider;
import :api.provider;
import :provider.item;
import liorm.sqlito;
import rstd.json;
import liuuid;
export import rstd;
template<class T>
using AsyncMutex = rstd::async::Mutex<T>;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::JoinHandle;
namespace radio_watch = rstd::async::watch;
using rstd::collections::HashMap;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using rstd::time::Duration;
using rstd::time::Instant;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
auto radio_now() -> i64 {
    return i64(Instant::now().duration_since_epoch().as_millis().to_primitive());
}
enum class RadioOperationIdStatus
{
    Valid,
    Invalid,
    Expired
};
auto radio_operation_id(ref<str> value) -> RadioOperationIdStatus {
    auto  bytes = value.as_bytes();
    usize separator;
    while (separator < bytes.len() && bytes[separator] != u8(':')) {
        if (bytes[separator] < u8('0') || bytes[separator] > u8('9'))
            return RadioOperationIdStatus::Invalid;
        ++separator;
    }
    if (separator == bytes.len() || separator == usize()) return RadioOperationIdStatus::Invalid;
    auto timestamp = rstd::from_str<u64>(value.get(usize(), separator).unwrap());
    if (timestamp.is_err()) return RadioOperationIdStatus::Invalid;
    auto uuid = value.get(separator + usize(1), value.len()).unwrap();
    if (liuuid::Uuid::parse_str(uuid).is_err()) return RadioOperationIdStatus::Invalid;
    auto now = current_millis().to_primitive();
    if (now < 0 || *timestamp > u64(now) + u64(30000) ||
        (*timestamp <= u64(now) && u64(now) - *timestamp >= u64(300000)))
        return RadioOperationIdStatus::Expired;
    return RadioOperationIdStatus::Valid;
}
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
    bool        stopped_ {};
    static auto release(ProviderRuntime&, SyncCoordinator, sqlito::ExecutorHandle,
                        RadioSessionHandle) -> rstd::async::coro<empty>;
    auto        release_selected(ProviderRuntime&, SyncCoordinator&, sqlito::ExecutorHandle,
                                 ProviderHandle, Cleanup) -> rstd::async::coro<empty>;

public:
    auto is_stopped() const -> bool { return stopped_; }
    auto operation(ref<str> key, ref<str> fingerprint, ProviderHandle instance, u64 epoch)
        -> Result<Arc<RadioOperation>, RadioError> {
        if (stopped_) return Err(RadioError::Expired());
        auto        guard = operations_.lock().unwrap();
        auto        now   = radio_now();
        Vec<String> expired;
        for (auto entry : guard->iter()) {
            const auto& op = *rstd::get<1>(entry);
            if (now - op->created >= i64(300000) &&
                (op->result.is_some() || op.strong_count() == usize(1)))
                expired.push(rstd::get<0>(entry)->clone());
        }
        for (auto& id : expired) guard->remove(id);
        auto found = guard->get(key);
        if (found.is_some()) {
            auto op = (*found)->clone();
            if (! ProviderHandle::ptr_eq(op->instance, instance) || op->epoch != epoch)
                return Err(RadioError::Expired());
            if (op->fingerprint.as_str() != fingerprint) return Err(RadioError::Conflict());
            return Ok(rstd::move(op));
        }
        if (guard->len() >= usize(512))
            return Err(RadioError::Invalid(String::make("too many radio operations"_str)));
        auto op =
            Arc<RadioOperation>::make(String::make(fingerprint), rstd::move(instance), epoch, now);
        guard->insert(String::make(key), op.clone());
        return Ok(rstd::move(op));
    }
    auto adopt(RadioSession session) -> RadioSessionHandle {
        auto owner = make_rc<RadioSession>(rstd::move(session));
        auto guard = sessions_.lock().unwrap();
        guard->retain([](const String&, RadioSessionHandle& session) {
            return session->status != RadioSessionStatus::Released;
        });
        // A colliding candidate must not displace a session that still owns cleanup.
        if (! guard->contains_key(owner->id)) guard->insert(owner->id.clone(), owner.clone());
        return owner;
    }
    auto publish(RadioSessionHandle owner) -> bool {
        auto guard = sessions_.lock().unwrap();
        if (stopped_ || ! owner || owner->status != RadioSessionStatus::Pending) return false;
        auto indexed = guard->get(owner->id);
        if (indexed.is_none() || ! RadioSessionHandle::ptr_eq(**indexed, owner)) return false;
        usize active;
        for (auto session : guard->values())
            if (session->get()->status == RadioSessionStatus::Active) ++active;
        if (active >= usize(128)) return false;
        owner.get()->status = RadioSessionStatus::Active;
        return true;
    }
    auto snapshot(ref<str> id) -> RadioSessionHandle {
        if (stopped_) return {};
        auto guard = sessions_.lock().unwrap();
        auto found = guard->get(id);
        if (found.is_none()) return {};
        const auto& session = **found;
        if (session->status == RadioSessionStatus::Active &&
            radio_now() - session->touched < i64(30 * 60 * 1000))
            return session.clone();
        return {};
    }
    auto peek(ref<str> id) -> RadioSessionHandle {
        auto guard = sessions_.lock().unwrap();
        auto found = guard->get(id);
        if (found.is_some() && (*found)->get()->status != RadioSessionStatus::Released)
            return (*found)->clone();
        return {};
    }
    auto start_close(ProviderRuntime& runtime, SyncCoordinator& coordinator,
                     sqlito::ExecutorHandle db, RadioSessionHandle session) -> void;
    auto close(ProviderRuntime&, SyncCoordinator&, sqlito::ExecutorHandle, String)
        -> rstd::async::coro<empty>;
    auto collect(ProviderRuntime& runtime, SyncCoordinator& coordinator, sqlito::ExecutorHandle db)
        -> rstd::async::coro<empty> {
        return release_selected(runtime, coordinator, rstd::move(db), {}, Cleanup::Expired);
    }
    auto invalidate(ProviderRuntime& runtime, SyncCoordinator& coordinator,
                    sqlito::ExecutorHandle db, ProviderHandle provider)
        -> rstd::async::coro<empty> {
        return release_selected(
            runtime, coordinator, rstd::move(db), rstd::move(provider), Cleanup::Provider);
    }
    auto shutdown(ProviderRuntime& runtime, SyncCoordinator& coordinator, sqlito::ExecutorHandle db)
        -> rstd::async::coro<empty> {
        stopped_ = true;
        return release_selected(runtime, coordinator, rstd::move(db), {}, Cleanup::All);
    }
};
auto radio_batch_ids(const RadioBatch& batch, bool opening) -> Option<Vec<i64>> {
    if (batch.state.len() > usize(65536)) return None();
    Vec<i64> ids;
    bool     playable = false;
    for (const auto& song : batch.songs.as_slice()) {
        if (song.id <= i64()) return None();
        ids.push(i64(song.id));
        playable = playable || song.can_play;
    }
    if (opening && ! playable) return None();
    return Some(rstd::move(ids));
}
auto radio_page(const RadioSession& session, Vec<i64> song_ids) -> RadioPage {
    return { session.id.clone(),           session.version,
             rstd::move(song_ids),         session.configuration.clone(),
             session.presentation.clone(), session.exhausted };
}
auto queue_session_message(const RadioPage& page) -> Box<p_qcm::p_msg::p_model::M_QueueSession> {
    auto message                = Box<p_qcm::p_msg::p_model::M_QueueSession>::make();
    message->f_id               = page.session_id.clone();
    message->f_version          = page.version;
    message->f_configuration    = page.configuration.clone();
    auto presentation           = Box<p_qcm::p_msg::p_model::M_QueuePresentation>::make();
    presentation->f_description = page.presentation.description.clone();
    presentation->f_source_item_id =
        page.presentation.source_item_id.is_some() ? *page.presentation.source_item_id : i64();
    message->f_presentation = Some(rstd::move(presentation));
    message->f_exhausted    = page.exhausted;
    return message;
}
auto song_ids_message(const Vec<i64>& result) -> Box<p_qcm::p_msg::p_id::M_SongIds> {
    auto ids = Box<p_qcm::p_msg::p_id::M_SongIds>::make();
    for (const auto id : result.as_slice()) ids->f_values.push(i64(id));
    return ids;
}
auto radio_page_response(i32 request_id, const RadioPage& page, bool opening)
    -> p_qcm::p_msg::M_QcmMessage {
    if (opening) {
        auto rsp        = Box<p_qcm::p_msg::M_OpenQueueRsp>::make();
        rsp->f_song_ids = Some(song_ids_message(page.song_ids));
        rsp->f_session  = Some(queue_session_message(page));
        return qcm_message(request_id, rstd::move(rsp));
    } else {
        auto rsp        = Box<p_qcm::p_msg::M_GetQueueNextRsp>::make();
        rsp->f_song_ids = Some(song_ids_message(page.song_ids));
        rsp->f_session  = Some(queue_session_message(page));
        return qcm_message(request_id, rstd::move(rsp));
    }
}

struct RadioOpenPlan {
    RadioQueueRow      row;
    i64                provider_id {};
    ProviderHandle     instance;
    u64                epoch {};
    Option<QueueInput> input;
};
auto prepare_radio_open(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                        const p_qcm::p_msg::M_OpenQueueReq& entry)
    -> rstd::async::coro<Result<RadioOpenPlan, ProcessError>> {
    if (entry.f_queue_id.is_none())
        co_return Err(ProcessError::MissingFields(String::make("queue_id"_str)));
    if ((*entry.f_queue_id)->f_value <= i64())
        co_return Err(ProcessError::WrongId(rstd::format("{}", (*entry.f_queue_id)->f_value)));
    auto queue = rstd_co_try(
        co_await database_run(db.clone(),
                              [queue_id = (*entry.f_queue_id)->f_value](
                                  sqlito::Connection& connection) -> ProcessResult<RadioQueueRow> {
                                  auto row = rstd_try(radio_queue_row(connection, queue_id));
                                  if (row.is_none()) return Err(ProcessError::NotFound());
                                  return Ok(rstd::move(*row));
                              }));
    RadioOpenPlan plan;
    plan.provider_id = queue.provider_id;
    plan.row         = rstd::move(queue);
    plan.instance    = runtime.acquire(plan.provider_id);
    if (! plan.instance)
        co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", plan.provider_id)));
    auto epoch = plan.instance->session_epoch();
    if (epoch.is_none()) co_return Err(ProcessError::radio(RadioError::Expired()));
    plan.epoch = *epoch;
    if (entry.f_reason < i32() || entry.f_reason > i32(2))
        co_return Err(ProcessError::MissingFields(String::make("invalid radio reason"_str)));
    if (entry.f_input_id != i64()) {
        plan.input = Some(rstd_co_try(co_await database_run(
            db.clone(),
            [input_id = entry.f_input_id, provider_id = plan.provider_id](
                sqlito::Connection& connection) -> ProcessResult<QueueInput> {
                auto item = read_item_by_id(connection, input_id);
                if (item.is_err())
                    return Err(rstd::into<ProcessError>(rstd::move(item).unwrap_err()));
                if (item->is_none()) return Err(ProcessError::NotFound());
                if ((**item).provider_id != provider_id)
                    return Err(ProcessError::WrongId(rstd::format("{}", input_id)));
                return Ok(QueueInput { rstd::move((**item).native_id), (**item).type });
            })));
    }
    co_return Ok(rstd::move(plan));
}
auto radio_current_songs(sqlito::Connection& db, i64 provider_id, slice<i64> current_ids)
    -> Result<Vec<String>, ProcessError> {
    Vec<String> result;
    for (const auto id : current_ids) {
        if (id <= i64()) return Err(ProcessError::WrongId(rstd::format("{}", id)));
        auto item = read_item_by_id(db, id);
        if (item.is_err()) return Err(rstd::into<ProcessError>(rstd::move(item).unwrap_err()));
        if (item->is_some() && (**item).provider_id == provider_id &&
            (**item).type == ItemType::Song)
            result.push((**item).native_id.clone());
    }
    return Ok(rstd::move(result));
}
struct RadioBatchItems {
    ItemRetentionHandle   retention;
    QueryResult<Vec<i64>> songs;
};
auto retain_radio_items(sqlito::Connection& db, SyncCoordinator& coordinator, i64 provider_id,
                        slice<i64> roots, slice<i64> requested)
    -> Result<RadioBatchItems, sqlito::Error> {
    auto retained = rstd_try(retain_items(coordinator, db, provider_id, roots));
    auto songs    = radio_ordered_songs(db, provider_id, requested);
    return Ok(RadioBatchItems { rstd::move(retained), rstd::move(songs) });
}
auto RadioSessions::release(ProviderRuntime& runtime, SyncCoordinator coordinator,
                            sqlito::ExecutorHandle db, RadioSessionHandle session)
    -> rstd::async::coro<empty> {
    struct Completion {
        RadioSessionHandle session;
        ~Completion() {
            (void)session.get()->retention.take();
            session.get()->status = RadioSessionStatus::Released;
            (void)session.get()->released.send_replace(true);
        }
    } completion { session.clone() };
    auto guard = co_await session->gate.lock();
    if (! session->instance) co_return empty {};
    auto instance = session->instance.clone();
    auto alive    = [instance = instance.clone()] {
        return instance->session_epoch().is_some();
    };
    auto writable =
        [call = runtime.call_snapshot(session->provider_id, instance, Some(u64(session->epoch)))] {
            return call.is_some() && call->is_current();
        };
    ProviderContext context(
        db.clone(), coordinator.clone(), session->provider_id, session->sync.clone());
    context.set_valid(rstd::move(alive));
    context.set_writable(rstd::move(writable));
    auto result = co_await rstd::async::timeout(
        rstd::async::into_future(instance.get()->close_queue_async(
            rstd::move(context), session->native_id.clone(), session->state.clone())),
        Duration::from_secs(u64(10)));
    if (result.is_err() || result->is_err()) rstd::log::warn("radio session cleanup failed");
    co_return empty {};
}
auto RadioSessions::start_close(ProviderRuntime& runtime, SyncCoordinator& coordinator,
                                sqlito::ExecutorHandle db, RadioSessionHandle session) -> void {
    {
        auto guard = sessions_.lock().unwrap();
        if (session->status == RadioSessionStatus::Closing ||
            session->status == RadioSessionStatus::Released)
            return;
        session.get()->status = RadioSessionStatus::Closing;
    }
    // The slot owns cleanup independently of a cancelled request or close waiter.
    session.get()->closing = Some(rstd::async::spawn_local(
        release(runtime, coordinator.clone(), rstd::move(db), session.clone())));
}
auto wait_radio_session_closed(RadioSessionHandle session) -> rstd::async::coro<empty> {
    auto receiver = session->released.subscribe();
    while (! *receiver.borrow_and_update()) {
        auto changed = co_await receiver.changed();
        if (changed.is_err()) break;
    }
    co_return empty {};
}
auto RadioSessions::close(ProviderRuntime& runtime, SyncCoordinator& coordinator,
                          sqlito::ExecutorHandle db, String id) -> rstd::async::coro<empty> {
    auto session = peek(id.as_str());
    if (! session) co_return empty {};
    start_close(runtime, coordinator, rstd::move(db), session.clone());
    co_await wait_radio_session_closed(session.clone());
    auto guard   = sessions_.lock().unwrap();
    auto indexed = guard->get(id.as_str());
    if (indexed.is_some() && RadioSessionHandle::ptr_eq(**indexed, session))
        (void)guard->remove(id.as_str());
    co_return empty {};
}
auto RadioSessions::release_selected(ProviderRuntime& runtime, SyncCoordinator& coordinator,
                                     sqlito::ExecutorHandle db, ProviderHandle provider,
                                     Cleanup cleanup) -> rstd::async::coro<empty> {
    if (cleanup == Cleanup::Provider && ! provider) co_return empty {};
    Vec<RadioSessionHandle> closing;
    {
        auto guard = sessions_.lock().unwrap();
        auto now   = radio_now();
        guard->retain([](const String&, RadioSessionHandle& session) {
            return session->status != RadioSessionStatus::Released;
        });
        for (auto value : guard->values()) {
            const auto& session = *value;
            if (cleanup == Cleanup::Provider &&
                ! ProviderHandle::ptr_eq(session->instance, provider))
                continue;
            if (cleanup == Cleanup::Expired && ! stopped_ &&
                session->status != RadioSessionStatus::Closing) {
                auto epoch = session->instance ? session->instance->session_epoch() : Option<u64>();
                if (epoch.is_some() && *epoch == session->epoch &&
                    runtime.find(session->provider_id) == session->instance.get() &&
                    now - session->touched < i64(30 * 60 * 1000))
                    continue;
            }
            closing.push(session.clone());
        }
    }
    for (const auto& session : closing.as_slice())
        start_close(runtime, coordinator, db.clone(), session.clone());
    for (auto& session : closing) co_await wait_radio_session_closed(rstd::move(session));
    {
        auto guard = sessions_.lock().unwrap();
        guard->retain([](const String&, RadioSessionHandle& session) {
            return session->status != RadioSessionStatus::Released;
        });
    }
    {
        auto        guard = operations_.lock().unwrap();
        auto        now   = radio_now();
        Vec<String> expired;
        for (auto entry : guard->iter()) {
            const auto& operation = *rstd::get<1>(entry);
            if (operation->result.is_some() &&
                (stopped_ || now - operation->created >= i64(300000) ||
                 (provider && ProviderHandle::ptr_eq(operation->instance, provider))))
                expired.push(rstd::get<0>(entry)->clone());
        }
        for (auto& key : expired) (void)guard->remove(key);
    }
    co_return empty {};
}
class RadioOpeningCandidate {
    ProviderRuntime&       runtime_;
    RadioSessions&         sessions_;
    SyncCoordinator&       coordinator_;
    sqlito::ExecutorHandle db_;
    RadioSessionHandle     session_;

public:
    RadioOpeningCandidate(ProviderRuntime& runtime, RadioSessions& sessions,
                          SyncCoordinator& coordinator, sqlito::ExecutorHandle db,
                          RadioSessionHandle session)
        : runtime_(runtime),
          sessions_(sessions),
          coordinator_(coordinator),
          db_(rstd::move(db)),
          session_(rstd::move(session)) {}
    RadioOpeningCandidate(const RadioOpeningCandidate&)                    = delete;
    auto operator=(const RadioOpeningCandidate&) -> RadioOpeningCandidate& = delete;
    ~RadioOpeningCandidate() {
        if (session_->status == RadioSessionStatus::Pending)
            sessions_.start_close(runtime_, coordinator_, db_.clone(), session_.clone());
    }
};
auto finish_radio_open(ProviderRuntime& runtime, RadioSessions& sessions,
                       SyncCoordinator& coordinator, sqlito::ExecutorHandle db,
                       RadioSessionHandle session, RadioBatch batch)
    -> rstd::async::coro<RadioResult> {
    auto guard = co_await session->gate.lock();
    auto valid = [&] {
        auto current = session->instance->session_epoch();
        return session->status == RadioSessionStatus::Pending &&
               runtime.find(session->provider_id) == session->instance.get() && current.is_some() &&
               *current == session->epoch;
    };
    if (! valid()) co_return Err(RadioError::Expired());
    auto requested = radio_batch_ids(batch, true);
    if (requested.is_none())
        co_return Err(RadioError::Invalid(String::make("invalid queue batch"_str)));
    Vec<i64> roots;
    for (auto id : requested->as_slice()) roots.push(i64(id));
    if (batch.presentation.source_item_id.is_some())
        roots.push(i64(*batch.presentation.source_item_id));
    auto retained = co_await database_run(
        db.clone(),
        [coordinator = coordinator.clone(),
         provider_id = session->provider_id,
         roots       = rstd::move(roots),
         requested   = rstd::move(*requested)](sqlito::Connection& connection) mutable {
            return retain_radio_items(
                connection, coordinator, provider_id, roots.as_slice(), requested.as_slice());
        });
    if (! valid()) co_return Err(RadioError::Expired());
    auto items               = rstd_co_try(rstd::move(retained), [](sqlito::Error error) {
        return RadioError(ProviderError(rstd::move(error)));
    });
    session.get()->retention = Some(rstd::move(items.retention));
    auto songs               = rstd_co_try(rstd::move(items.songs), [](QueryError error) {
        return RadioError(ProviderError::db(rstd::move(error)));
    });
    if (songs.is_empty())
        co_return Err(RadioError::Invalid(String::make("radio has no songs"_str)));
    session.get()->configuration = rstd::move(batch.configuration);
    session.get()->presentation  = rstd::move(batch.presentation);
    session.get()->exhausted     = batch.exhausted;
    auto page                    = radio_page(*session, rstd::move(songs));
    if (! sessions.publish(session))
        co_return Err(RadioError::Invalid(String::make("too many radio sessions"_str)));
    co_return Ok(rstd::move(page));
}
auto perform_open_queue_async(ProviderRuntime& runtime, RadioSessions& sessions,
                              SyncCoordinator& coordinator, sqlito::ExecutorHandle db,
                              const p_qcm::p_msg::M_OpenQueueReq& entry, RadioOpenPlan plan,
                              ProviderRequestOwnerHandle owner = {})
    -> rstd::async::coro<RadioResult> {
    auto  provider_id = plan.provider_id;
    auto  instance    = plan.instance.clone();
    auto  epoch       = plan.epoch;
    auto& row         = plan.row;
    if (sessions.is_stopped()) co_return Err(RadioError::Expired());
    if (owner) {
        owner.get()->provider_id = provider_id;
        owner.get()->instance    = instance.clone();
    }
    auto call = runtime.call_snapshot(provider_id, instance, Some(epoch));
    if (call.is_none()) co_return Err(RadioError::Expired());
    auto valid = [snapshot = call->clone()] {
        return snapshot.is_current();
    };
    Option<String> previous_state;
    if (! entry.f_previous_session_id.is_empty()) {
        auto previous = sessions.snapshot(entry.f_previous_session_id.as_str());
        if (! previous) co_return Err(RadioError::Expired());
        auto guard = co_await previous->gate.lock();
        if (! valid() || previous->status != RadioSessionStatus::Active ||
            radio_now() - previous->touched >= i64(30 * 60 * 1000) ||
            previous->queue_id != (*entry.f_queue_id)->f_value ||
            ! ProviderHandle::ptr_eq(previous->instance, instance) || previous->epoch != epoch)
            co_return Err(RadioError::Expired());
        previous_state = Some(previous->state.clone());
    }
    auto configuration = entry.f_configuration.clone();
    auto context       = call->context(db.clone(), coordinator.clone());
    auto definition =
        co_await instance.get()->queue_definition_async(context.clone(), row.native_id.clone());
    if (! valid() || sessions.is_stopped()) co_return Err(RadioError::Expired());
    auto queue_definition = rstd_co_try(rstd::move(definition));
    for (const auto& field : queue_definition.configuration.as_slice())
        if (! configuration.contains_key(field.key))
            configuration.insert(field.key.clone(), field.value.clone());
    auto sync   = context.sync.clone();
    auto reason = entry.f_reason == i32(1)   ? QueueOpenReason::Reconfigure
                  : entry.f_reason == i32(2) ? QueueOpenReason::Restart
                                             : QueueOpenReason::Initial;
    auto opened =
        co_await instance.get()->open_queue_async(rstd::move(context),
                                                  row.native_id.clone(),
                                                  QueueOpenRequest { rstd::move(configuration),
                                                                     rstd::move(plan.input),
                                                                     reason,
                                                                     rstd::move(previous_state) });
    if (opened.is_err() && ! valid()) co_return Err(RadioError::Expired());
    auto         result = rstd_co_try(rstd::move(opened));
    RadioSession session;
    session.id                      = new_instance_id();
    session.queue_id                = (*entry.f_queue_id)->f_value;
    session.native_id               = row.native_id.clone();
    session.provider_id             = provider_id;
    session.state                   = result.state.clone();
    session.instance                = instance.clone();
    session.epoch                   = epoch;
    session.sync                    = rstd::move(sync);
    session.touched                 = radio_now();
    session.version                 = u64(1);
    auto                  candidate = sessions.adopt(rstd::move(session));
    RadioOpeningCandidate pending(runtime, sessions, coordinator, db.clone(), candidate.clone());
    auto                  page = co_await finish_radio_open(
        runtime, sessions, coordinator, db.clone(), candidate.clone(), rstd::move(result));
    if (page.is_err()) {
        sessions.start_close(runtime, coordinator, db.clone(), candidate.clone());
        co_await wait_radio_session_closed(candidate.clone());
    }
    co_return rstd::move(page);
}

auto perform_next_queue_async(ProviderRuntime& runtime, RadioSessions& sessions,
                              SyncCoordinator& coordinator, sqlito::ExecutorHandle db,
                              const p_qcm::p_msg::M_GetQueueNextReq& entry,
                              Vec<String> current_native_ids, ProviderRequestOwnerHandle owner = {})
    -> rstd::async::coro<RadioResult> {
    auto session = sessions.snapshot(entry.f_session_id.as_str());
    if (! session) co_return Err(RadioError::Expired());
    if (owner) {
        owner.get()->provider_id = session->provider_id;
        owner.get()->instance    = session->instance.clone();
    }
    auto guard = co_await session->gate.lock();
    if (session->status != RadioSessionStatus::Active ||
        radio_now() - session->touched >= i64(30 * 60 * 1000))
        co_return Err(RadioError::Expired());
    auto current_instance = runtime.acquire(session->provider_id);
    auto epoch            = session->instance ? session->instance->session_epoch() : Option<u64>();
    if (! current_instance || ! ProviderHandle::ptr_eq(current_instance, session->instance) ||
        epoch.is_none() || *epoch != session->epoch)
        co_return Err(RadioError::Expired());
    if (entry.f_queue_id.is_some() && (*entry.f_queue_id)->f_value != session->queue_id)
        co_return Err(RadioError::Expired());
    if (entry.f_expected_version != session->version) co_return Err(RadioError::Conflict());
    if (session->exhausted) co_return Err(RadioError::Invalid(String::make("radio has ended"_str)));
    auto call =
        runtime.call_snapshot(session->provider_id, session->instance, Some(u64(session->epoch)));
    if (call.is_none()) co_return Err(RadioError::Expired());
    auto valid = [snapshot = call->clone(), session = session.clone()] {
        return session->status == RadioSessionStatus::Active && snapshot.is_current();
    };
    auto context = call->context(db.clone(), coordinator.clone());
    context.restrict_valid([session = session.clone()] {
        return session->status == RadioSessionStatus::Active;
    });
    Vec<i64> previous_roots;
    if (session->retention.is_some()) {
        for (auto id : (*session->retention)->roots()) previous_roots.push(i64(id));
    }
    auto retained_roots_result = co_await database_run(
        db.clone(),
        [provider_id = session->provider_id,
         roots       = rstd::move(previous_roots),
         current     = current_native_ids.clone()](
            sqlito::Connection& connection) -> ProviderResult<Vec<i64>> {
            Vec<i64> retained_roots;
            for (auto id : roots.as_slice()) {
                auto item = read_item_by_id(connection, id);
                if (item.is_err())
                    return Err(rstd::into<ProviderError>(rstd::move(item).unwrap_err()));
                if (item->is_none() || (**item).provider_id != provider_id ||
                    (**item).type != ItemType::Song)
                    continue;
                for (const auto& native_id : current.as_slice())
                    if ((**item).native_id == native_id) {
                        retained_roots.push(i64(id));
                        break;
                    }
            }
            return Ok(rstd::move(retained_roots));
        });
    if (! valid()) co_return Err(RadioError::Expired());
    auto retained_roots = rstd_co_try(rstd::move(retained_roots_result));
    auto next           = co_await session.get()->instance.get()->next_queue_async(
        rstd::move(context),
        session->native_id.clone(),
        QueueNextRequest { session->state.clone(), rstd::move(current_native_ids) });
    if (next.is_err() && ! valid()) co_return Err(RadioError::Expired());
    auto result = rstd_co_try(rstd::move(next));
    // Remote advancement survives failed local validation or retention.
    session.get()->state = result.state.clone();
    if (! valid()) co_return Err(RadioError::Expired());
    auto requested = radio_batch_ids(result, false);
    if (requested.is_none())
        co_return Err(RadioError::Invalid(String::make("invalid queue batch"_str)));
    if (session->configuration != result.configuration)
        co_return Err(RadioError::Invalid(
            String::make("radio configuration changed during continuation"_str)));
    for (auto id : requested->as_slice()) retained_roots.push(i64(id));
    if (result.presentation.source_item_id.is_some())
        retained_roots.push(i64(*result.presentation.source_item_id));
    auto retained = co_await database_run(
        db.clone(),
        [coordinator = coordinator.clone(),
         provider_id = session->provider_id,
         roots       = rstd::move(retained_roots),
         requested   = rstd::move(*requested)](sqlito::Connection& connection) mutable {
            return retain_radio_items(
                connection, coordinator, provider_id, roots.as_slice(), requested.as_slice());
        });
    if (! valid()) co_return Err(RadioError::Expired());
    auto batch_items             = rstd_co_try(rstd::move(retained), [](sqlito::Error error) {
        return RadioError(ProviderError(rstd::move(error)));
    });
    auto songs                   = rstd_co_try(rstd::move(batch_items.songs), [](QueryError error) {
        return RadioError(ProviderError::db(rstd::move(error)));
    });
    session.get()->retention     = Some(rstd::move(batch_items.retention));
    session.get()->presentation  = rstd::move(result.presentation);
    session.get()->exhausted     = result.exhausted;
    session.get()->configuration = rstd::move(result.configuration);
    session.get()->version       = session->version + u64(1);
    session.get()->touched       = radio_now();
    co_return Ok(radio_page(*session, rstd::move(songs)));
}

auto close_queue_async(ProviderRuntime& runtime, RadioSessions& sessions,
                       SyncCoordinator& coordinator, sqlito::ExecutorHandle db, i32 request_id,
                       const p_qcm::p_msg::M_CloseQueueReq& entry,
                       ProviderRequestOwnerHandle           owner = {})
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>> {
    auto session = sessions.peek(entry.f_session_id.as_str());
    if (session && owner) {
        owner.get()->provider_id = session->provider_id;
        owner.get()->instance    = session->instance.clone();
    }
    co_await sessions.close(runtime, coordinator, db.clone(), entry.f_session_id.clone());
    auto rsp       = Box<p_qcm::p_msg::M_Rsp>::make();
    rsp->f_code    = i32();
    rsp->f_message = String::make();
    co_return Ok(qcm_message(request_id, rstd::move(rsp)));
}
auto radio_operation_id_error(RadioOperationIdStatus status) -> RadioError {
    if (status == RadioOperationIdStatus::Expired) return RadioError::Expired();
    return RadioError::Invalid(String::make("invalid radio operation id"_str));
}
auto radio_append_text(String& text, ref<str> value) -> void {
    text.push_str("\""_str);
    text.push_str(json_escape(value).as_str());
    text.push_str("\""_str);
}
auto radio_open_fingerprint(const p_qcm::p_msg::M_OpenQueueReq& entry, const RadioOpenPlan& plan)
    -> String {
    auto value = rstd::format("[{},{},{},",
                              (*entry.f_queue_id)->f_value.to_primitive(),
                              plan.provider_id.to_primitive(),
                              entry.f_reason.to_primitive());
    radio_append_text(value, entry.f_previous_session_id.as_str());
    value.push_str(","_str);
    if (plan.input.is_some()) {
        value.push_str("["_str);
        radio_append_text(value, plan.input->native_id.as_str());
        value.push_str(","_str);
        radio_append_text(value, rstd::format("{}", plan.input->item_type).as_str());
        value.push_str("]"_str);
    } else
        value.push_str("null"_str);
    value.push_str(",["_str);
    bool first = true;
    for (auto item : entry.f_configuration.iter()) {
        if (! first) value.push_str(","_str);
        first = false;
        value.push_str("["_str);
        radio_append_text(value, rstd::get<0>(item)->as_str());
        value.push_str(","_str);
        radio_append_text(value, rstd::get<1>(item)->as_str());
        value.push_str("]"_str);
    }
    value.push_str("]]"_str);
    return value;
}
auto radio_next_fingerprint(u64 version, const Vec<String>& ids) -> String {
    auto value = rstd::format("[{},[", version.to_primitive());
    bool first = true;
    for (const auto& id : ids.as_slice()) {
        if (! first) value.push_str(","_str);
        first = false;
        radio_append_text(value, id.as_str());
    }
    value.push_str("]]"_str);
    return value;
}
template<class Execute>
auto run_radio_operation(ProviderRuntime& runtime, RadioSessions& sessions, i64 provider_id,
                         ProviderHandle instance, u64 epoch, String key, String fingerprint,
                         Execute execute) -> rstd::async::coro<RadioResult> {
    auto operation =
        rstd_co_try(sessions.operation(key.as_str(), fingerprint.as_str(), instance, epoch));
    auto guard = co_await operation->mutex.lock();
    auto valid = [&] {
        auto current = instance->session_epoch();
        return ! sessions.is_stopped() && runtime.find(provider_id) == instance.get() &&
               current.is_some() && *current == epoch;
    };
    if (! valid()) co_return Err(RadioError::Expired());
    if (operation->result.is_none()) {
        auto result = co_await rstd::async::timeout(rstd::async::into_future(execute()),
                                                    Duration::from_secs(u64(180)));
        if (result.is_err()) {
            operation->result = Some(RadioResult(
                Err(RadioError::Invalid(String::make("radio operation timed out"_str)))));
        } else {
            operation->result = Some(rstd::move(result).unwrap());
        }
    }
    if (! valid()) co_return Err(RadioError::Expired());
    const auto& result  = *operation->result;
    const auto& page    = rstd_co_try(result.as_ref(), [](const auto& error) {
        return error.clone();
    });
    auto        session = sessions.snapshot(page.session_id.as_str());
    if (! session || ! ProviderHandle::ptr_eq(session->instance, instance) ||
        session->epoch != epoch)
        co_return Err(RadioError::Expired());
    co_return Ok(page.clone());
}
auto open_queue_async(ProviderRuntime& runtime, RadioSessions& sessions,
                      SyncCoordinator& coordinator, sqlito::ExecutorHandle db, i32 request_id,
                      const p_qcm::p_msg::M_OpenQueueReq& entry,
                      ProviderRequestOwnerHandle          owner = {})
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>> {
    auto plan   = rstd_co_try(co_await prepare_radio_open(runtime, db.clone(), entry));
    auto status = radio_operation_id(entry.f_operation_id.as_str());
    if (status != RadioOperationIdStatus::Valid)
        co_return Err(ProcessError::radio(radio_operation_id_error(status)));
    if (owner) {
        owner.get()->provider_id = plan.provider_id;
        owner.get()->instance    = plan.instance.clone();
    }
    auto key = String::make("open/"_str);
    key.push_str(entry.f_operation_id.as_str());
    auto fingerprint = radio_open_fingerprint(entry, plan);
    auto page        = rstd_co_try(co_await run_radio_operation(
        runtime,
        sessions,
        plan.provider_id,
        plan.instance,
        plan.epoch,
        rstd::move(key),
        rstd::move(fingerprint),
        [&] {
            return perform_open_queue_async(
                runtime, sessions, coordinator, db.clone(), entry, rstd::move(plan), owner);
        }));
    co_return Ok(radio_page_response(request_id, page, true));
}
auto legacy_next_queue_async(ProviderRuntime& runtime, SyncCoordinator& coordinator,
                             sqlito::ExecutorHandle db, i32 request_id,
                             const p_qcm::p_msg::M_GetQueueNextReq& entry, Item target,
                             Vec<String> current_ids, ProviderRequestOwnerHandle owner)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>> {
    auto instance = runtime.acquire(target.provider_id);
    if (! instance)
        co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", target.provider_id)));
    auto epoch = instance->session_epoch();
    if (epoch.is_none()) co_return Err(ProcessError::NotAuth());
    if (owner) {
        owner.get()->provider_id = target.provider_id;
        owner.get()->instance    = instance.clone();
    }
    auto call = runtime.call_snapshot(target.provider_id, instance, epoch);
    if (call.is_none()) co_return Err(ProcessError::NotAuth());
    auto valid = [snapshot = call->clone()] {
        return snapshot.is_current();
    };
    auto context = call->context(db.clone(), coordinator.clone());
    auto batch   = co_await instance.get()->legacy_queue_async(rstd::move(context),
                                                               target.native_id.clone(),
                                                               rstd::move(current_ids),
                                                               entry.f_configuration.clone());
    if (! valid()) co_return Err(ProcessError::NotAuth());
    auto     result = rstd_co_try(rstd::move(batch), [](ProviderError error) {
        return ProcessError::radio(RadioError(rstd::move(error)));
    });
    Vec<i64> requested;
    for (const auto& song : result.songs.as_slice()) requested.push(i64(song.id));
    auto ids = co_await database_run(
        db.clone(),
        [provider_id = target.provider_id,
         requested   = rstd::move(requested)](sqlito::Connection& connection) {
            return radio_ordered_songs(connection, provider_id, requested.as_slice());
        });
    if (! valid()) co_return Err(ProcessError::NotAuth());
    auto ordered_songs = rstd_co_try(rstd::move(ids));
    auto values        = Box<p_qcm::p_msg::p_id::M_SongIds>::make();
    values->f_values   = rstd::move(ordered_songs);
    auto rsp           = Box<p_qcm::p_msg::M_GetQueueNextRsp>::make();
    rsp->f_song_ids    = Some(rstd::move(values));
    co_return Ok(qcm_message(request_id, rstd::move(rsp)));
}
auto next_queue_async(ProviderRuntime& runtime, RadioSessions& sessions,
                      SyncCoordinator& coordinator, sqlito::ExecutorHandle db, i32 request_id,
                      const p_qcm::p_msg::M_GetQueueNextReq& entry,
                      ProviderRequestOwnerHandle             owner = {})
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>> {
    if (entry.f_current_song_ids.is_some())
        for (auto id : (*entry.f_current_song_ids)->f_values.as_slice())
            if (id <= i64()) co_return Err(ProcessError::WrongId(rstd::format("{}", id)));
    if (entry.f_queue_id.is_none())
        co_return Err(ProcessError::MissingFields(String::make("queue_id"_str)));
    if ((*entry.f_queue_id)->f_value <= i64())
        co_return Err(ProcessError::WrongId(rstd::format("{}", (*entry.f_queue_id)->f_value)));
    auto target = rstd_co_try(co_await database_run(
        db.clone(),
        [queue_id = (*entry.f_queue_id)->f_value](
            sqlito::Connection& connection) -> ProcessResult<Option<Item>> {
            auto item = read_item_by_id(connection, queue_id);
            if (item.is_err()) return Err(rstd::into<ProcessError>(rstd::move(item).unwrap_err()));
            return Ok(rstd::move(item).unwrap());
        }));
    if (target.is_none()) {
        if (entry.f_session_id.is_empty()) co_return Err(ProcessError::NotFound());
        co_return Err(ProcessError::radio(RadioError::Expired()));
    }
    if (target->type != ItemType::Radio)
        co_return Err(ProcessError::UnsupportedItemType(rstd::format("{}", target->type)));
    Vec<i64> current_song_ids;
    if (entry.f_current_song_ids.is_some())
        current_song_ids = (*entry.f_current_song_ids)->f_values.clone();
    if (entry.f_session_id.is_empty()) {
        auto ids = rstd_co_try(co_await database_run(
            db.clone(),
            [provider_id = target->provider_id,
             ids         = rstd::move(current_song_ids)](sqlito::Connection& connection) {
                return radio_current_songs(connection, provider_id, ids.as_slice());
            }));
        co_return co_await legacy_next_queue_async(runtime,
                                                   coordinator,
                                                   db.clone(),
                                                   request_id,
                                                   entry,
                                                   rstd::move(*target),
                                                   rstd::move(ids),
                                                   owner);
    }
    auto session = sessions.snapshot(entry.f_session_id.as_str());
    if (! session || session->queue_id != (*entry.f_queue_id)->f_value ||
        session->provider_id != target->provider_id || ! session->instance)
        co_return Err(ProcessError::radio(RadioError::Expired()));
    auto epoch = session->instance->session_epoch();
    if (runtime.find(session->provider_id) != session->instance.get() || epoch.is_none() ||
        *epoch != session->epoch)
        co_return Err(ProcessError::radio(RadioError::Expired()));
    auto ids    = rstd_co_try(co_await database_run(
        db.clone(),
        [provider_id = session->provider_id,
         ids         = rstd::move(current_song_ids)](sqlito::Connection& connection) {
            return radio_current_songs(connection, provider_id, ids.as_slice());
        }));
    auto status = radio_operation_id(entry.f_operation_id.as_str());
    if (status != RadioOperationIdStatus::Valid)
        co_return Err(ProcessError::radio(radio_operation_id_error(status)));
    if (owner) {
        owner.get()->provider_id = session->provider_id;
        owner.get()->instance    = session->instance.clone();
    }
    auto key = String::make("next/"_str);
    key.push_str(entry.f_session_id.as_str());
    key.push_str("/"_str);
    key.push_str(entry.f_operation_id.as_str());
    auto fingerprint = radio_next_fingerprint(entry.f_expected_version, ids);
    auto page        = rstd_co_try(co_await run_radio_operation(
        runtime,
        sessions,
        session->provider_id,
        session->instance,
        session->epoch,
        rstd::move(key),
        rstd::move(fingerprint),
        [&] {
            return perform_next_queue_async(
                runtime, sessions, coordinator, db.clone(), entry, rstd::move(ids), owner);
        }));
    co_return Ok(radio_page_response(request_id, page, false));
}
} // namespace qcm::backend
