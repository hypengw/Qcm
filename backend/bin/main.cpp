#include <rstd/macro.hpp>
import qcm.backend;
import rstd.log;
import rstd;
import sqlito;
using rstd::async::coro;
using rstd::async::RuntimeBuilder;
using rstd::async::RuntimeHandle;
using rstd::ffi::OsString;
using rstd::net::SocketAddr;
using rstd::net::TcpListener;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;
using namespace qcm::backend;

namespace
{
auto display(const auto& report) -> int {
    if (report.target() == rstd::argparse::OutputTarget::Tag::Stdout) {
        auto output = rstd::io::stdout();
        (void)rstd::io::write_all(output, report.text().as_bytes());
        (void)rstd::as<rstd::io::Write>(output).flush();
    } else {
        auto output = rstd::io::stderr();
        (void)rstd::io::write_all(output, report.text().as_bytes());
        (void)rstd::as<rstd::io::Write>(output).flush();
    }
    return report.exit_code().to_primitive();
}
auto fail(ref<str> message) -> int {
    auto error = rstd::io::stderr();
    (void)rstd::io::write_all(error, message.as_bytes());
    (void)rstd::as<rstd::io::Write>(error).flush();
    return 1;
}
auto report_ui(rstd::async::Child child) -> coro<empty> {
    auto status = co_await child.wait();
    if (status.is_err())
        rstd::log::error("qcm-ui wait failed: {}", status.as_ref().unwrap_err());
    else if (! status->success()) {
        if (auto code = status->code(); code.is_some())
            rstd::log::warn("qcm-ui exited with code {}", *code);
        else if (auto signal = status->signal(); signal.is_some())
            rstd::log::warn("qcm-ui terminated by signal {}", *signal);
    }
    co_return empty {};
}

auto activate_ui(const startup::Instance& instance, const PathBuf& program,
                 const startup::Options& options) -> coro<rstd::io::Result<int>> {
    auto ready = rstd_co_try(co_await startup::wait_ready(instance));
    if (options.no_ui) co_return Ok(0);
    auto supervisor = rstd_co_try(rstd::async::ProcessSupervisor::make());
    auto command    = startup::ui_command(program.as_path(), ready, options.ui_args.as_slice());
    auto child      = supervisor.handle().spawn(command);
    if (child.is_err()) {
        (void)co_await supervisor.shutdown();
        co_return Err(rstd::move(child).unwrap_err());
    }
    auto status = co_await child->wait();
    auto closed = co_await supervisor.shutdown();
    rstd_co_try(closed);
    auto exited = rstd_co_try(rstd::move(status));
    co_return Ok(exited.code().unwrap_or(i32(1)).to_primitive());
}

auto launch_and_serve(TcpListener listener, Arc<QcmService> service, liserveto::Shutdown shutdown,
                      startup::Instance& instance, const PathBuf& program,
                      const startup::Options& options, const startup::Ready& ready)
    -> coro<rstd::io::Result<empty>> {
    auto supervisor = rstd_co_try(rstd::async::ProcessSupervisor::make());
    auto published  = instance.publish(ready);
    if (published.is_err()) {
        (void)co_await supervisor.shutdown();
        co_return Err(rstd::move(published).unwrap_err());
    }
    Option<rstd::async::AbortOnDropHandle<empty>> ui_task;
    if (! options.no_ui) {
        auto command = startup::ui_command(program.as_path(), ready, options.ui_args.as_slice());
        auto child   = supervisor.handle().spawn(command);
        if (child.is_err()) {
            (void)co_await supervisor.shutdown();
            co_return Err(rstd::move(child).unwrap_err());
        }
        ui_task = Some(rstd::async::AbortOnDropHandle<empty> {
            rstd::async::spawn_local(report_ui(rstd::move(*child))) });
    }
    rstd::log::info("qcm listening on 127.0.0.1:{}", ready.port);
    auto served = co_await serve_qcm(rstd::move(listener), service.clone(), rstd::move(shutdown));
    if (ui_task.is_some()) {
        ui_task->abort();
        (void)co_await rstd::move(*ui_task);
    }
    auto closed = co_await supervisor.shutdown();
    if (served.is_err()) co_return Err(rstd::move(served).unwrap_err());
    rstd_co_try(closed);
    co_return Ok(empty {});
}
auto watch_shutdown_signal(rstd::signal::Subscription subscription, liserveto::Shutdown shutdown)
    -> rstd::async::coro<empty> {
    auto receive = rstd::async::signal::recv(subscription);
    if (receive.is_err()) {
        rstd::log::error("cannot wait for shutdown signal: {}", receive.as_ref().unwrap_err());
    } else {
        auto event = co_await rstd::move(receive).unwrap();
        if (event.is_err())
            rstd::log::error("shutdown signal failed: {}", event.as_ref().unwrap_err());
        else if (event->is_some())
            rstd::log::info("qcm shutdown signal {}", (**event).as_raw());
        else {
            rstd::log::error("shutdown signal subscription closed");
            co_return empty {};
        }
    }
    shutdown.request_stop();
    co_return empty {};
}
auto run_backend(RuntimeHandle runtime, TcpListener listener, String main_path, String cache_path,
                 ProviderRegistry registry, String cache_dir, String device_id,
                 liserveto::Shutdown shutdown, rstd::signal::Subscription signals,
                 startup::Instance& instance, const PathBuf& program,
                 const startup::Options& options, const startup::Ready& ready)
    -> rstd::async::coro<Result<empty, ProcessError>> {
    auto signal_task = rstd::async::AbortOnDropHandle<empty> { rstd::async::spawn_local(
        watch_shutdown_signal(rstd::move(signals), shutdown.clone())) };
    auto database    = rstd_co_try(co_await MainDatabase::open(main_path.clone()));
    auto db          = rstd_co_try(database.handle());
    auto initialized = co_await database_run(db.clone(), [](sqlito::Connection& connection) {
        return migrate(connection);
    });
    if (initialized.is_err()) {
        (void)co_await database.shutdown();
        co_return Err(ProcessError(rstd::move(initialized).unwrap_err()));
    }
    auto restored = co_await restore_network_settings(db.clone());
    if (restored.is_err()) {
        (void)co_await database.shutdown();
        co_return Err(rstd::move(restored).unwrap_err());
    }
    auto cache = sqlito::Executor::open(cache_path.as_str());
    if (cache.is_err()) {
        (void)co_await database.shutdown();
        co_return Err(ProcessError(rstd::move(cache).unwrap_err()));
    }
    auto cache_database = rstd::move(cache).unwrap();
    auto cache_initialized =
        co_await database_run(cache_database.handle(),
                              [](sqlito::Connection& connection) -> Result<empty, sqlito::Error> {
                                  rstd_try(connection.execute_batch(
                                      "PRAGMA journal_mode=WAL;"
                                      "PRAGMA synchronous=NORMAL;PRAGMA busy_timeout=5000;"_str));
                                  rstd_try(migrate_cache(connection));
                                  return Ok(empty {});
                              });
    if (cache_initialized.is_err()) {
        (void)co_await cache_database.shutdown();
        (void)co_await database.shutdown();
        co_return Err(ProcessError(rstd::move(cache_initialized).unwrap_err()));
    }
    auto created =
        co_await QcmService::make(db.clone(),
                                  rstd::move(runtime),
                                  rstd::move(registry),
                                  Some(cache_database.handle()),
                                  Some(rstd::move(cache_dir)),
                                  device_id.is_empty() ? None() : Some(rstd::move(device_id)),
                                  Some(rstd::move(main_path)));
    Result<empty, ProcessError> result = Ok(empty {});
    if (created.is_err()) {
        result = Err(rstd::move(created).unwrap_err());
    } else {
        auto started = co_await (*created)->start_resource_cache();
        if (started.is_err()) {
            co_await (*created)->stop_providers();
            result = Err(rstd::move(started).unwrap_err());
        } else {
            auto served = co_await launch_and_serve(rstd::move(listener),
                                                    (*created).clone(),
                                                    rstd::move(shutdown),
                                                    instance,
                                                    program,
                                                    options,
                                                    ready);
            if (served.is_err())
                result = Err(ProcessError::internal(rstd::move(served).unwrap_err()));
            (void)co_await (*created)->stop_resource_cache();
            co_await (*created)->stop_providers();
        }
    }
    auto cache_closed = co_await cache_database.shutdown();
    auto main_closed  = co_await database.shutdown();
    signal_task.abort();
    (void)co_await rstd::move(signal_task);
    rstd_co_try(result);
    rstd_co_try(cache_closed);
    rstd_co_try(main_closed);
    co_return Ok(empty {});
}
} // namespace

auto main(int argc, char** argv) -> int {
    static auto logger = rstd::log::EnvLogger {};
    (void)rstd::log::set_logger(logger);
    rstd::log::set_max_level(logger.filter());
    rstd::env::args_init(argc, argv);
    auto arguments  = rstd::iter::from_iter<Vec<OsString>>(rstd::env::args_os());
    auto executable = arguments.is_empty() ? PathBuf::from("qcm"_str)
                                           : PathBuf::from(arguments[usize()].as_os_str());
    auto parsed     = startup::parse(rstd::move(arguments));
    if (parsed.is_err()) return display(parsed.as_ref().unwrap_err());
    if (parsed->is_Display()) return display(parsed->as_Display().request);
    auto options = rstd::move(*parsed).as_Parsed().value;
    auto paths   = startup::storage_paths(options);
    if (paths.is_err()) return fail(rstd::format("{}\n", paths.as_ref().unwrap_err()).as_str());
    if (rstd::fs::create_dir_all(paths->data.as_path()).is_err())
        return fail("cannot create data directory\n"_str);
    if (rstd::fs::create_dir_all(paths->cache.as_path()).is_err())
        return fail("cannot create cache directory\n"_str);
    auto data_dir   = rstd::fs::canonicalize(paths->data.as_path());
    auto cache_root = rstd::fs::canonicalize(paths->cache.as_path());
    if (data_dir.is_err() || cache_root.is_err())
        return fail("cannot resolve storage directories\n"_str);
    auto instance = startup::Instance::open(data_dir->as_path());
    if (instance.is_err())
        return fail(
            rstd::format("cannot lock backend: {}\n", instance.as_ref().unwrap_err()).as_str());
#ifdef __linux__
    auto resolved = rstd::fs::read_link(PathBuf::from("/proc/self/exe"_str).as_path());
    if (resolved.is_ok()) executable = rstd::move(*resolved);
#endif
    auto program = options.ui.is_some() ? PathBuf::from(options.ui->as_str())
                                        : startup::ui_program(executable.as_path());
    auto runtime = RuntimeBuilder::current_thread().enable_all().build();
    if (runtime.is_err()) return fail("cannot create runtime\n"_str);
    if (! instance->primary()) {
        auto activated = runtime->block_on(activate_ui(*instance, program, options));
        if (activated.is_err())
            return fail(
                rstd::format("cannot activate qcm-ui: {}\n", activated.as_ref().unwrap_err())
                    .as_str());
        return *activated;
    }
    auto cache_dir = cache_root->join("QcmBackend"_str);
    if (rstd::fs::create_dir_all(cache_dir.as_path()).is_err())
        return fail("cannot create resource cache directory\n"_str);
    auto main_path  = data_dir->join("backend.2.db"_str);
    auto cache_path = data_dir->join("backend_cache.db"_str);
    auto bind       = TcpListener::bind(SocketAddr::ipv4_loopback(options.port));
    if (bind.is_err()) return fail("cannot bind backend port\n"_str);
    auto listener = rstd::move(*bind);
    auto address  = listener.local_addr();
    if (address.is_err()) return fail("cannot read backend address\n"_str);
    startup::Ready ready { address->port(),
                           String::make(cache_root->as_path().to_string_lossy().as_str()) };
    auto           setting = settings::Setting::load(data_dir->as_path());
    if (setting.is_err()) return fail(rstd::format("{}\n", setting.as_ref().unwrap_err()).as_str());
    auto device_id = rstd::move(setting->device_id);

    auto registry = LuaProviderPlugin::load_default();
    if (registry.is_err()) {
        rstd::log::error("cannot load provider registry: {}", registry.as_ref().unwrap_err());
        return fail("cannot load provider registry\n"_str);
    }
    auto shutdown = liserveto::Shutdown::make();
    if (shutdown.is_err()) return fail("cannot create shutdown handle\n"_str);
    Vec<rstd::signal::SignalKind> kinds;
    kinds.push(rstd::signal::SignalKind::interrupt());
#ifndef _WIN32
    auto terminate = rstd::signal::unix::terminate();
    if (terminate.is_err()) return fail("cannot select shutdown signal\n"_str);
    kinds.push(rstd::move(*terminate));
#endif
    auto signals = rstd::signal::subscribe(kinds.as_slice());
    if (signals.is_err()) return fail("cannot subscribe to shutdown signals\n"_str);
    auto result =
        runtime->block_on(run_backend(runtime->handle(),
                                      rstd::move(listener),
                                      String::make(main_path.as_path().to_string_lossy().as_str()),
                                      String::make(cache_path.as_path().to_string_lossy().as_str()),
                                      rstd::move(*registry),
                                      String::make(cache_dir.as_path().to_string_lossy().as_str()),
                                      rstd::move(device_id),
                                      rstd::move(shutdown).unwrap(),
                                      rstd::move(signals).unwrap(),
                                      *instance,
                                      program,
                                      options,
                                      ready));
    if (result.is_err()) {
        rstd::log::error("backend failed: {}", result.as_ref().unwrap_err());
        return fail("backend server failed\n"_str);
    }
    return 0;
}
