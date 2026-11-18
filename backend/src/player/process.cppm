export module qcm.backend:player.process;
import qcm.protocol;
import rstd;
using alloc::rc::Rc;
using rstd::async::coro;
using rstd::path::PathBuf;
using rstd::time::Duration;
using namespace rstd::prelude;

export namespace qcm::backend
{
using PlayerProcessResult = Result<empty, String>;
using PlayerEventHandler  = Box<dyn<Fn<coro<empty>(qcm::player_ipc::Event)>>>;

struct PlayerProcessOptions {
    usize    command_capacity { 64 };
    Duration ready_timeout { Duration::from_secs(u64(10)) };
    Duration write_timeout { Duration::from_secs(u64(2)) };
    Duration shutdown_timeout { Duration::from_secs(u64(3)) };
};

// All handles and callbacks belong to the backend's current-thread runtime.
// Keep that runtime running until shutdown completes.
class PlayerProcess {
    struct WriteRequest {
        String                                            frame;
        qcm::player_ipc::Action                           action;
        u64                                               epoch;
        rstd::async::oneshot::Sender<PlayerProcessResult> completed;
    };
    struct Shared {
        rstd::async::mpsc::Sender<WriteRequest> commands;
        PlayerEventHandler                      on_event;
        PlayerProcessOptions                    options;
        rstd::async::watch::Sender<u64>         changed { u64() };
        Option<rstd::async::JoinHandle<empty>>  owner;
        Option<String>                          failure;
        Option<i32>                             pid;
        qcm::player_ipc::State                  latest;
        u64                                     epoch {}, revision {};
        bool ready {}, close_requested {}, finished {}, shutdown_sent {};

        Shared(rstd::async::mpsc::Sender<WriteRequest> commands, PlayerEventHandler handler,
               PlayerProcessOptions options);
        auto stopping() const -> bool;
        void notify();
        void close();
        void fail(String error);
        auto closed_error() const -> String;
    };
    struct Driver;
    struct ConstructionKey {};
    Rc<Shared> shared_;

public:
    explicit PlayerProcess(ConstructionKey, Rc<Shared> shared);
    PlayerProcess(const PlayerProcess&)                    = delete;
    auto operator=(const PlayerProcess&) -> PlayerProcess& = delete;
    ~PlayerProcess();

    static auto launch(PathBuf program, PlayerEventHandler handler,
                       PlayerProcessOptions options = {}) -> Result<Rc<PlayerProcess>, String>;

    static auto start(PathBuf program, PlayerEventHandler handler,
                      PlayerProcessOptions options = {}) -> coro<Result<Rc<PlayerProcess>, String>>;
    auto        wait_ready() const -> coro<PlayerProcessResult>;
    // Success acknowledges a complete pipe write, not the player's Ack or operation completion.
    auto send(qcm::player_ipc::Command command) const -> coro<PlayerProcessResult>;
    auto shutdown() const -> coro<empty>;
    auto id() const -> Option<i32>;
    auto is_ready() const -> bool;
    auto is_closed() const -> bool;
    auto is_finished() const -> bool;
};
} // namespace qcm::backend
