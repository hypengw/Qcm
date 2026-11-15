export module qcm.backend:system.ui;
export import :session;
export import :startup;
import rstd.log;

using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::async::ProcessHandle;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::atomic::Atomic;
using rstd::time::Duration;
using rstd::time::Instant;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
class UiLauncher {
    ProcessHandle                 processes_;
    PathBuf                       program_;
    startup::Ready                ready_;
    Vec<String>                   arguments_;
    Arc<SessionRegistry>          sessions_;
    Arc<Atomic<bool>>             running_ { Arc<Atomic<bool>>::make(false) };
    Vec<AbortOnDropHandle<empty>> jobs_;

    static auto report(rstd::async::Child child, Arc<Atomic<bool>> running) -> coro<empty> {
        struct Finished {
            Arc<Atomic<bool>> running;
            ~Finished() { running->store(false); }
        } finished { rstd::move(running) };
        auto status = co_await child.wait();
        if (status.is_err())
            rstd::log::error("qcm-ui wait failed: {}", status.unwrap_err());
        else if (! status->success()) {
            if (auto code = status->code(); code.is_some())
                rstd::log::warn("qcm-ui exited with code {}", *code);
            else if (auto signal = status->signal(); signal.is_some())
                rstd::log::warn("qcm-ui terminated by signal {}", *signal);
        }
        co_return empty {};
    }

public:
    UiLauncher(ProcessHandle processes, PathBuf program, startup::Ready ready,
               Vec<String> arguments, Arc<SessionRegistry> sessions)
        : processes_(rstd::move(processes)),
          program_(rstd::move(program)),
          ready_(rstd::move(ready)),
          arguments_(rstd::move(arguments)),
          sessions_(rstd::move(sessions)) {}

    auto open(String token = {}) -> coro<Result<empty, String>> {
        auto activated = co_await sessions_->activate_ui(rstd::move(token));
        if (activated.is_err())
            co_return Err(rstd::format("cannot activate qcm-ui: {}", activated.unwrap_err()));
        const auto waiting = Instant::now();
        while (running_->load() && sessions_->active_session().is_none()) {
            if (waiting.elapsed() >= Duration::from_secs(u64(5)))
                co_return Err("qcm-ui did not connect to the backend"_Str);
            co_await rstd::async::sleep(Duration::from_millis(u64(25)));
        }
        if (sessions_->active_session().is_some()) co_return Ok(empty {});
        jobs_.retain([](const auto& job) {
            return ! job.is_finished();
        });
        auto command = startup::ui_command(program_.as_path(), ready_, arguments_.as_slice());
        command.env_remove("XDG_ACTIVATION_TOKEN"_str).env_remove("DESKTOP_STARTUP_ID"_str);
        auto child = processes_.spawn(command);
        if (child.is_err())
            co_return Err(rstd::format("cannot start qcm-ui: {}", child.unwrap_err()));
        running_->store(true);
        jobs_.push(AbortOnDropHandle<empty> {
            rstd::async::spawn_local(report(rstd::move(*child), running_.clone())) });
        co_return Ok(empty {});
    }

    auto finish() -> coro<empty> {
        for (auto& job : jobs_) job.abort();
        for (auto& job : jobs_) (void)co_await rstd::move(job);
        jobs_.clear();
        co_return empty {};
    }
};
} // namespace qcm::backend
