module;
#include <rstd/macro.hpp>
export module qcm.backend:player.process;
export import rstd;
export import qcm.protocol;

using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::async::JoinHandle;
using rstd::async::mpsc::Receiver;
using rstd::async::mpsc::Sender;
using rstd::future::by_ref;
using rstd::path::PathBuf;
using rstd::time::Duration;
using rstd::time::Instant;
using namespace rstd::prelude;
using namespace rstd::literals;

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
        Sender<WriteRequest>            commands;
        PlayerEventHandler              on_event;
        PlayerProcessOptions            options;
        rstd::async::watch::Sender<u64> changed { u64() };
        Option<JoinHandle<empty>>       owner;
        Option<String>                  failure;
        Option<i32>                     pid;
        qcm::player_ipc::State          latest;
        u64                             epoch {}, revision {};
        bool                            ready {}, close_requested {}, finished {}, shutdown_sent {};

        Shared(Sender<WriteRequest> commands, PlayerEventHandler handler,
               PlayerProcessOptions options)
            : commands(rstd::move(commands)), on_event(rstd::move(handler)), options(options) {}
        auto stopping() const -> bool { return close_requested || failure.is_some(); }
        void notify() { (void)changed.send_replace(++revision); }
        void close() {
            if (close_requested || finished) return;
            close_requested = true;
            notify();
        }
        void fail(String error) {
            if (failure.is_some()) return;
            failure = Some(rstd::move(error));
            notify();
        }
        auto closed_error() const -> String {
            return failure.is_some() ? failure->clone() : "player process is closed"_Str;
        }
    };
    struct Pipes {
        rstd::async::pipe::Sender   input;
        rstd::async::pipe::Receiver output;
        Pipes(rstd::async::pipe::Sender input, rstd::async::pipe::Receiver output)
            : input(rstd::move(input)), output(rstd::move(output)) {}
    };
    struct Completion {
        Rc<Shared> shared;
        Completion(Rc<Shared> shared): shared(rstd::move(shared)) {}
        Completion(const Completion&)     = delete;
        Completion(Completion&&) noexcept = default;
        ~Completion() {
            if (! shared) return;
            shared.get()->finished = true;
            // The lifecycle owns itself until its children and supervisor are joined.
            shared.get()->owner = None();
            shared.get()->notify();
        }
    };
    enum class Wait
    {
        Ready,
        Stop,
        Close,
        Finished
    };
    struct ConstructionKey {};
    Rc<Shared> shared_;

    static auto wait(Rc<Shared> shared, Wait condition) -> coro<empty> {
        auto changed = shared->changed.subscribe();
        for (;;) {
            bool done {};
            switch (condition) {
            case Wait::Ready: done = shared->ready || shared->stopping() || shared->finished; break;
            case Wait::Stop: done = shared->stopping() || shared->finished; break;
            case Wait::Close: done = shared->close_requested || shared->finished; break;
            case Wait::Finished: done = shared->finished; break;
            }
            if (done || (co_await changed.changed()).is_err()) co_return empty {};
        }
    }

    static auto read_events(Rc<Shared> shared, Rc<Pipes> pipes) -> coro<empty> {
        qcm::player_ipc::JsonLines lines;
        for (;;) {
            auto incoming = co_await rstd::async::select(
                rstd::async::into_future(wait(shared.clone(), Wait::Stop)),
                rstd::async::into_future(pipes->output.read(usize(16384))));
            if (incoming.is_left()) co_return empty {};
            auto bytes = rstd::move(incoming).unwrap_right();
            if (bytes.is_err()) {
                shared.get()->fail(
                    rstd::format("player stdout read failed: {}", bytes.unwrap_err()));
                co_return empty {};
            }
            if (bytes->is_empty()) {
                auto ended = lines.finish();
                if (! shared->stopping())
                    shared.get()->fail(ended.is_err() ? rstd::move(ended).unwrap_err()
                                                      : "player stdout closed unexpectedly"_Str);
                co_return empty {};
            }
            auto frames = lines.feed(bytes->as_slice());
            if (frames.is_err()) {
                shared.get()->fail(rstd::move(frames).unwrap_err());
                co_return empty {};
            }
            for (const auto& frame : *frames) {
                if (shared->stopping()) co_return empty {};
                auto decoded = qcm::player_ipc::decode_event(frame.as_str());
                if (decoded.is_err()) {
                    shared.get()->fail(rstd::move(decoded).unwrap_err());
                    co_return empty {};
                }
                auto event = rstd::move(decoded).unwrap();
                if ((! shared->ready && event.kind != qcm::player_ipc::EventKind::Ready) ||
                    (shared->ready && event.kind == qcm::player_ipc::EventKind::Ready) ||
                    (event.kind == qcm::player_ipc::EventKind::Error && event.id == u64())) {
                    shared.get()->fail("invalid player event handshake or error id"_Str);
                    co_return empty {};
                }
                shared.get()->latest = event.state.clone();
                if (event.state.epoch > shared->epoch) shared.get()->epoch = event.state.epoch;
                if (event.kind == qcm::player_ipc::EventKind::Ready) {
                    shared.get()->ready = true;
                    shared.get()->notify();
                }
                auto delivered = co_await rstd::async::select(
                    rstd::async::into_future(wait(shared.clone(), Wait::Stop)),
                    rstd::async::into_future(shared->on_event->operator()(rstd::move(event))));
                if (delivered.is_left()) co_return empty {};
            }
        }
    }

    static auto write_frame(Rc<Pipes> pipes, String frame, Duration timeout)
        -> coro<PlayerProcessResult> {
        auto result = co_await rstd::async::timeout(
            rstd::async::into_future(pipes->input.write_all(
                rstd::bytes::Bytes::from_vec(rstd::move(frame).into_bytes()))),
            timeout);
        if (result.is_err()) co_return Err("player stdin write timed out"_Str);
        if (result->is_err())
            co_return Err(rstd::format("player stdin write failed: {}", result->unwrap_err()));
        co_return Ok(empty {});
    }

    static auto write_commands(Rc<Shared> shared, Rc<Pipes> pipes, Receiver<WriteRequest> commands)
        -> coro<empty> {
        bool writable = true;
        for (;;) {
            auto next = co_await rstd::async::select(
                rstd::async::into_future(wait(shared.clone(), Wait::Stop)),
                rstd::async::into_future(commands.recv()));
            if (next.is_left()) break;
            auto pending = rstd::move(next).unwrap_right();
            if (pending.is_none()) break;
            auto request = pending.take().unwrap();
            // A frame must finish before another frame or Shutdown can be written.
            auto written = co_await write_frame(
                pipes.clone(), rstd::move(request.frame), shared->options.write_timeout);
            if (written.is_err()) {
                writable = false;
                shared.get()->fail(written.as_ref().unwrap_err().clone());
                (void)request.completed.send(rstd::move(written));
                break;
            }
            if (request.action == qcm::player_ipc::Action::Source && request.epoch > shared->epoch)
                shared.get()->epoch = request.epoch;
            if (request.action == qcm::player_ipc::Action::Shutdown) {
                shared.get()->shutdown_sent = true;
                shared.get()->close();
            }
            (void)request.completed.send(Ok(empty {}));
        }
        commands.close();
        if (writable && ! shared->shutdown_sent) {
            qcm::player_ipc::Command command;
            command.action   = qcm::player_ipc::Action::Shutdown;
            command.epoch    = shared->epoch;
            command.autoplay = false;
            auto encoded     = qcm::player_ipc::encode_command(command);
            if (encoded.is_ok()) {
                auto written = co_await write_frame(
                    pipes.clone(), rstd::move(encoded).unwrap(), shared->options.write_timeout);
                if (written.is_err() && ! shared->close_requested)
                    shared.get()->fail(rstd::move(written).unwrap_err());
            } else {
                shared.get()->fail(rstd::move(encoded).unwrap_err());
            }
        }
        co_return empty {};
    }

    static auto watch_child(Rc<Shared>                                        shared,
                            coro<rstd::io::Result<rstd::process::ExitStatus>> pending)
        -> coro<empty> {
        auto status = co_await rstd::move(pending);
        if (! shared->stopping()) {
            if (status.is_err())
                shared.get()->fail(rstd::format("player wait failed: {}", status.unwrap_err()));
            else if (auto code = status->code())
                shared.get()->fail(rstd::format("player exited unexpectedly with code {}", *code));
            else
                shared.get()->fail(rstd::format("player exited unexpectedly with signal {}",
                                                status->signal().unwrap_or(i32())));
        }
        co_return empty {};
    }

    static auto report_failure(Rc<Shared> shared) -> coro<empty> {
        if (shared->failure.is_none()) co_return empty {};
        qcm::player_ipc::Event event;
        event.id                   = u64();
        event.kind                 = qcm::player_ipc::EventKind::Error;
        event.state                = shared->latest.clone();
        event.state.epoch          = shared->epoch;
        event.state.sequence       = event.state.sequence.saturating_add(u64(1));
        event.state.playback_state = i32(2);
        event.state.busy           = false;
        event.state.seekable       = false;
        event.state.error          = shared->failure->clone();
        // An explicit close can cancel even a blocked fatal-event handler.
        (void)co_await rstd::async::timeout(
            rstd::async::select(
                rstd::async::into_future(wait(shared.clone(), Wait::Close)),
                rstd::async::into_future(shared->on_event->operator()(rstd::move(event)))),
            shared->options.write_timeout);
        co_return empty {};
    }

    static auto run(Completion completion, PathBuf program, Receiver<WriteRequest> commands,
                    rstd::async::ProcessSupervisor supervisor) -> coro<empty> {
        auto shared = completion.shared.clone();
        if (shared->stopping()) {
            commands.close();
            (void)co_await supervisor.shutdown();
            co_return empty {};
        }
        auto command = rstd::process::Command::make(program.as_path().as_os_str());
        command.set_stdin(rstd::process::Stdio::piped())
            .set_stdout(rstd::process::Stdio::piped())
            .set_stderr(rstd::process::Stdio::inherit());
        auto spawned = supervisor.handle().spawn(command);
        if (spawned.is_err()) {
            commands.close();
            shared.get()->fail(rstd::format("cannot start player: {}", spawned.unwrap_err()));
            (void)co_await supervisor.shutdown();
            co_await report_failure(shared.clone());
            co_return empty {};
        }
        auto child        = rstd::move(spawned).unwrap();
        shared.get()->pid = Some(child.id());
        auto input        = child.take_stdin();
        auto output       = child.take_stdout();
        if (input.is_none() || output.is_none()) {
            commands.close();
            shared.get()->fail("player pipe setup failed"_Str);
            (void)child.start_kill();
            (void)co_await child.wait();
            (void)co_await supervisor.shutdown();
            co_await report_failure(shared.clone());
            co_return empty {};
        }
        auto pipes  = make_rc<Pipes>(input.take().unwrap(), output.take().unwrap());
        auto reader = AbortOnDropHandle<empty>(
            rstd::async::spawn_local(read_events(shared.clone(), pipes.clone())));
        auto writer = AbortOnDropHandle<empty>(rstd::async::spawn_local(
            write_commands(shared.clone(), pipes.clone(), rstd::move(commands))));
        auto exited = AbortOnDropHandle<empty>(
            rstd::async::spawn_local(watch_child(shared.clone(), child.wait())));
        auto ready = co_await rstd::async::timeout(
            rstd::async::into_future(wait(shared.clone(), Wait::Ready)),
            shared->options.ready_timeout);
        if (ready.is_err()) shared.get()->fail("player ready timed out"_Str);
        if (! shared->stopping()) co_await wait(shared.clone(), Wait::Stop);

        const auto closing = Instant::now();
        reader.abort();
        (void)co_await rstd::move(reader);
        auto written =
            co_await rstd::async::timeout(by_ref(writer), shared->options.shutdown_timeout);
        if (written.is_err()) {
            (void)child.start_kill();
            writer.abort();
            (void)co_await rstd::move(writer);
        }
        (void)co_await rstd::move(pipes.get()->input).close();
        auto stopped = co_await rstd::async::timeout(
            by_ref(exited), shared->options.shutdown_timeout.saturating_sub(closing.elapsed()));
        if (stopped.is_err()) {
            (void)child.start_kill();
            (void)co_await rstd::move(exited);
        }
        (void)co_await rstd::move(pipes.get()->output).close();
        auto supervised = co_await supervisor.shutdown();
        if (supervised.is_err() && ! shared->close_requested)
            shared.get()->fail(
                rstd::format("player supervision failed: {}", supervised.unwrap_err()));
        co_await report_failure(shared.clone());
        co_return empty {};
    }

    static auto send_owned(Rc<Shared> shared, qcm::player_ipc::Command command)
        -> coro<PlayerProcessResult> {
        if (shared->stopping() || shared->finished) co_return Err(shared->closed_error());
        if (! shared->ready) co_return Err("player process is not ready"_Str);
        auto encoded = qcm::player_ipc::encode_command(command);
        if (encoded.is_err()) co_return Err(rstd::move(encoded).unwrap_err());
        auto         completed = rstd::async::oneshot::channel<PlayerProcessResult>();
        WriteRequest request { rstd::move(encoded).unwrap(),
                               command.action,
                               command.epoch,
                               rstd::move(completed.get<0>()) };
        auto         queued = co_await rstd::async::select(
            rstd::async::into_future(wait(shared.clone(), Wait::Stop)),
            rstd::async::into_future(shared->commands.send(rstd::move(request))));
        if (queued.is_left() || rstd::move(queued).unwrap_right().is_err())
            co_return Err(shared->closed_error());
        auto written = co_await rstd::async::select(
            rstd::move(completed.get<1>()),
            rstd::async::into_future(wait(shared.clone(), Wait::Stop)));
        if (written.is_right()) co_return Err(shared->closed_error());
        auto result = rstd::move(written).unwrap_left();
        if (result.is_err()) co_return Err(shared->closed_error());
        co_return rstd::move(result).unwrap();
    }

    static auto wait_ready_owned(Rc<Shared> shared) -> coro<PlayerProcessResult> {
        co_await wait(shared.clone(), Wait::Ready);
        if (shared->ready && ! shared->stopping() && ! shared->finished) co_return Ok(empty {});
        co_await wait(shared.clone(), Wait::Finished);
        co_return Err(shared->closed_error());
    }

    static auto start_owned(PathBuf program, PlayerEventHandler handler,
                            PlayerProcessOptions options)
        -> coro<Result<Rc<PlayerProcess>, String>> {
        auto launched = launch(rstd::move(program), rstd::move(handler), options);
        if (launched.is_err()) co_return Err(rstd::move(launched).unwrap_err());
        auto process = rstd::move(launched).unwrap();
        auto ready   = co_await process->wait_ready();
        if (ready.is_err()) co_return Err(rstd::move(ready).unwrap_err());
        co_return Ok(rstd::move(process));
    }

public:
    explicit PlayerProcess(ConstructionKey, Rc<Shared> shared): shared_(rstd::move(shared)) {}
    PlayerProcess(const PlayerProcess&)                    = delete;
    auto operator=(const PlayerProcess&) -> PlayerProcess& = delete;
    ~PlayerProcess() { shared_.get()->close(); }

    static auto launch(PathBuf program, PlayerEventHandler handler,
                       PlayerProcessOptions options = {}) -> Result<Rc<PlayerProcess>, String> {
        if (options.command_capacity == usize()) return Err("player command capacity is zero"_Str);
        auto supervisor = rstd::async::ProcessSupervisor::make();
        if (supervisor.is_err())
            return Err(rstd::format("cannot supervise player: {}", supervisor.unwrap_err()));
        auto commands = rstd::async::mpsc::channel<WriteRequest>(options.command_capacity);
        auto shared  = make_rc<Shared>(rstd::move(commands.get<0>()), rstd::move(handler), options);
        auto process = make_rc<PlayerProcess>(ConstructionKey {}, shared.clone());
        shared.get()->owner = Some(rstd::async::spawn_local(run(Completion(shared.clone()),
                                                                rstd::move(program),
                                                                rstd::move(commands.get<1>()),
                                                                rstd::move(supervisor).unwrap())));
        return Ok(rstd::move(process));
    }

    static auto start(PathBuf program, PlayerEventHandler handler,
                      PlayerProcessOptions options = {})
        -> coro<Result<Rc<PlayerProcess>, String>> {
        return start_owned(rstd::move(program), rstd::move(handler), options);
    }
    auto wait_ready() const -> coro<PlayerProcessResult> {
        return wait_ready_owned(shared_.clone());
    }
    // Success acknowledges a complete pipe write, not the player's Ack or operation completion.
    auto send(qcm::player_ipc::Command command) const -> coro<PlayerProcessResult> {
        return send_owned(shared_.clone(), rstd::move(command));
    }
    auto shutdown() const -> coro<empty> {
        auto shared = shared_.clone();
        shared.get()->close();
        return wait(rstd::move(shared), Wait::Finished);
    }
    auto id() const -> Option<i32> { return shared_->pid; }
    auto is_ready() const -> bool { return shared_->ready && ! is_closed(); }
    auto is_closed() const -> bool { return shared_->stopping() || shared_->finished; }
    auto is_finished() const -> bool { return shared_->finished; }
};
} // namespace qcm::backend
