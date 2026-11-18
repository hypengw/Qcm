export module qcm.backend:player.session;
import :player.process;
import :player.queue;
import :player.preferences;
import qcm.backend.player.integration;
import :api.radio;
import :db.query.player;
import :db.sync.run;
import :db.timestamp;
import :error;
import :event;
import :playing;
import :protocol.message;
import :provider.queue;
import :provider.runtime;
import qcm.protocol;
import sqlito;
import rstd;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::async::mpsc::Receiver;
using rstd::async::mpsc::Sender;
using rstd::collections::BTreeMap;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using namespace rstd::prelude;
namespace ipc  = qcm::player_ipc;
namespace wire = p_qcm::p_msg;

export namespace qcm::backend
{
class PlayerSession {
    struct PlayerEvent {
        ipc::Event event;
        u64        generation, intent;
    };
    struct PendingAdvance {
        u64  intent, epoch;
        bool ended;
    };
    struct PlaylogEntry {
        i64           id;
        Timestamp     timestamp;
        PlayingAction action;
    };
    sqlito::ExecutorHandle              db_;
    Rc<ProviderRuntime>                 providers_;
    Arc<RadioSessions>                  radios_;
    SyncCoordinator                     coordinator_;
    PathBuf                             program_;
    String                              base_;
    EventSink                           events_;
    Weak<PlayerSession>                 self_ { Weak<PlayerSession>::make() };
    PlayerQueue                         queue_;
    ipc::State                          state_;
    Option<PlayerTrack>                 track_;
    Option<PathBuf>                     preferences_file_;
    PlayerPreferences                   saved_preferences_, configured_preferences_;
    Option<i64>                         source_song_, logged_song_;
    Option<u64>                         source_entry_id_;
    u64                                 logged_epoch_ {};
    u64                                 source_intent_ {}, radio_generation_ {};
    Option<Rc<PlayerProcess>>           process_;
    Option<Box<dyn<PlayerIntegration>>> integration_;
    rstd::async::Mutex<empty>           commands_ { empty {} };
    rstd::async::watch::Sender<u64>     changed_ { u64() };
    Option<AbortOnDropHandle<empty>>    publisher_;
    Option<AbortOnDropHandle<empty>>    prefetch_;
    Option<Sender<PlayerEvent>>         event_input_;
    Option<AbortOnDropHandle<empty>>    event_worker_;
    Option<Sender<PlaylogEntry>>        log_input_;
    Option<AbortOnDropHandle<empty>>    log_worker_;
    Option<PendingAdvance>              pending_advance_;
    Option<u64>                         prefetch_entry_;
    Vec<AbortOnDropHandle<empty>>       radio_closures_;
    u64  state_revision_ {}, queue_revision_ {}, epoch_ {}, process_generation_ {}, command_id_ {};
    u64  sequence_ {}, ended_serial_ {}, seek_serial_ {}, intent_ {};
    bool connected_ {}, process_failed_ {}, stopping_ {}, radio_exhausted_ {}, desired_playing_ {};
    String                   error_, name_, radio_session_;
    i64                      radio_id_ {};
    u64                      radio_version_ {};
    QueuePresentation        radio_presentation_;
    BTreeMap<String, String> radio_configuration_;

    void        changed(bool queue = false);
    void        report_error(String text);
    void        save_preferences();
    static auto publish(Weak<PlayerSession> weak, rstd::async::watch::Receiver<u64> changes)
        -> coro<empty>;
    static auto receive_events(Weak<PlayerSession> weak, Receiver<PlayerEvent> input)
        -> coro<empty>;
    void start_event_worker();
    auto ensure_process() -> coro<ProcessResult<empty>>;
    auto issue(u64 intent, ipc::Action action, double value = 0, String source = {},
               bool autoplay = true, Option<i64> song = None()) -> coro<ProcessResult<empty>>;
    static auto release_radio(Rc<ProviderRuntime> providers, Arc<RadioSessions> radios,
                              SyncCoordinator coordinator, sqlito::ExecutorHandle db, String id)
        -> coro<empty>;
    auto        close_radio() -> coro<empty>;
    auto        clear_source(u64 intent) -> coro<ProcessResult<empty>>;
    auto        select_source(u64 intent) -> coro<ProcessResult<empty>>;
    void        adopt_radio(const p_qcm::p_msg::p_model::M_QueueSession& session);
    auto        next_radio() -> coro<ProcessResult<empty>>;
    static auto prefetch(Weak<PlayerSession> weak) -> coro<empty>;
    void        maybe_prefetch(bool force = false);
    auto        advance(bool previous, bool ended, u64 intent, bool fetch = true)
        -> coro<ProcessResult<empty>>;
    auto        on_event(ipc::Event event, u64 generation, u64 observed_intent) -> coro<empty>;
    static auto record_logs(sqlito::ExecutorHandle db, Rc<ProviderRuntime> providers,
                            SyncCoordinator coordinator, EventSink events,
                            Receiver<PlaylogEntry> input) -> coro<empty>;
    auto        log(i64 id, PlayingAction action) -> coro<empty>;

public:
    PlayerSession(sqlito::ExecutorHandle db, Rc<ProviderRuntime> providers,
                  Arc<RadioSessions> radios, SyncCoordinator coordinator, PathBuf program,
                  String base, EventSink events, Option<PathBuf> preferences_file);

    static auto make(sqlito::ExecutorHandle db, Rc<ProviderRuntime> providers,
                     Arc<RadioSessions> radios, SyncCoordinator coordinator, PathBuf program,
                     String base, EventSink events, Option<PathBuf> preferences_file = None())
        -> Arc<PlayerSession>;
    auto state() const -> wire::M_PlayerStateMsg;
    auto queue_state() const -> wire::M_PlayerQueueMsg;
    auto start() -> coro<empty>;
    auto attach_integration(Box<dyn<PlayerIntegration>> integration)
        -> coro<PlayerIntegrationResult>;
    auto control(wire::M_PlayerControlReq request) -> coro<ProcessResult<empty>>;
    auto queue(wire::M_PlayerQueueReq request) -> coro<ProcessResult<empty>>;
    auto shutdown() -> coro<empty>;
};
} // namespace qcm::backend
