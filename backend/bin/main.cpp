#include <rstd/macro.hpp>
import qcm.backend;
import qcm.backend.startup_trace;
#if defined(__linux__)
import qcm.backend.system.mpris;
import qcm.backend.system.dbus;
import qcm.backend.system.tray;
#endif
import rstd.log;
import rstd;
import sqlito;
using alloc::rc::make_rc;
using rstd::async::coro;
using rstd::async::RuntimeBuilder;
using rstd::async::RuntimeHandle;
using rstd::ffi::OsString;
using rstd::net::SocketAddr;
using rstd::net::TcpListener;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using namespace rstd::prelude;
using namespace rstd::literals;
using namespace qcm::backend;

namespace
{
enum class StartupStage
{
    Total,
    Arguments,
    Paths,
    Instance,
    Runtime,
    Listener,
    Settings,
    Plugins,
    Shutdown,
    Signals,
    MainDatabaseOpen,
    MainDatabaseMigrate,
    NetworkRestore,
    CacheDatabaseOpen,
    CacheDatabaseMigrate,
    Service,
    Resources,
    Player,
    Publish,
    UiSpawn,
    InstanceWait,
    Count
};

constexpr ref<str> STARTUP_LABELS[] = {
    "total"_str,
    "arguments"_str,
    "paths"_str,
    "instance"_str,
    "runtime"_str,
    "listener"_str,
    "settings"_str,
    "plugins"_str,
    "shutdown"_str,
    "signals"_str,
    "database.main.open"_str,
    "database.main.migrate"_str,
    "network.restore"_str,
    "database.cache.open"_str,
    "database.cache.migrate"_str,
    "service.make"_str,
    "resource_cache.open"_str,
    "player.start"_str,
    "readiness.publish"_str,
    "ui.spawn"_str,
    "instance.wait"_str,
};
static_assert(sizeof(STARTUP_LABELS) / sizeof(STARTUP_LABELS[0]) ==
              static_cast<rstd::size_t>(StartupStage::Count));
using StartupTrace = startup::Trace<StartupStage>;

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
struct BackendPlayerHost {
    Weak<QcmService>    service;
    Arc<UiLauncher>     launcher;
    liserveto::Shutdown shutdown;

    auto control(p_qcm::p_msg::M_PlayerControlReq request) const -> coro<PlayerIntegrationResult> {
        auto owner = service.upgrade();
        if (! owner) co_return Err("player is unavailable"_Str);
        auto result = co_await owner->control_player(rstd::move(request));
        if (result.is_err()) co_return Err(rstd::format("{}", result.unwrap_err()));
        co_return Ok(empty {});
    }
    auto queue(p_qcm::p_msg::M_PlayerQueueReq request) const -> coro<PlayerIntegrationResult> {
        auto owner = service.upgrade();
        if (! owner) co_return Err("player is unavailable"_Str);
        auto result = co_await owner->edit_player_queue(rstd::move(request));
        if (result.is_err()) co_return Err(rstd::format("{}", result.unwrap_err()));
        co_return Ok(empty {});
    }
    auto open_ui(String token) const -> coro<PlayerIntegrationResult> {
        co_return co_await launcher->open(rstd::move(token));
    }
    auto raise(String token) const -> coro<PlayerIntegrationResult> {
        return open_ui(rstd::move(token));
    }
    auto quit() const -> coro<PlayerIntegrationResult> {
        (void)shutdown.request_stop();
        co_return Ok(empty {});
    }
};
} // namespace

namespace rstd
{
template<>
struct Impl<PlayerHost, BackendPlayerHost> : ImplBase<BackendPlayerHost> {
    auto control(p_qcm::p_msg::M_PlayerControlReq request) const -> coro<PlayerIntegrationResult> {
        return this->self().control(rstd::move(request));
    }
    auto queue(p_qcm::p_msg::M_PlayerQueueReq request) const -> coro<PlayerIntegrationResult> {
        return this->self().queue(rstd::move(request));
    }
    auto raise(String token) const -> coro<PlayerIntegrationResult> {
        return this->self().raise(rstd::move(token));
    }
    auto quit() const -> coro<PlayerIntegrationResult> { return this->self().quit(); }
};
#if defined(__linux__)
template<>
struct Impl<DaemonHost, BackendPlayerHost> : ImplBase<BackendPlayerHost> {
    auto open_ui(String token) const -> coro<PlayerIntegrationResult> {
        return this->self().open_ui(rstd::move(token));
    }
    auto control(p_qcm::p_msg::M_PlayerControlReq request) const -> coro<PlayerIntegrationResult> {
        return this->self().control(rstd::move(request));
    }
};
#endif
} // namespace rstd

namespace
{

struct DesktopSystem {
#if defined(__linux__)
    alloc::rc::Rc<DaemonBus> daemon;
    String                   icon_theme_path {};
    auto publish(Arc<QcmService> service, Arc<UiLauncher> launcher, liserveto::Shutdown shutdown,
                 const startup::Ready& ready) -> coro<rstd::io::Result<empty>> {
        PlayerIntegrations integrations;
        auto               desktop = co_await DesktopMpris::start(make_rc<dyn<PlayerHost>>(
            BackendPlayerHost { service.downgrade(), launcher.clone(), shutdown.clone() }));
        if (desktop.is_ok()) {
            integrations.add(Box<dyn<PlayerIntegration>>::make(rstd::move(*desktop)));
        } else {
            rstd::log::warn("MPRIS unavailable: {}", desktop.unwrap_err());
        }
        auto tray = co_await DesktopTray::start(
            make_rc<dyn<PlayerHost>>(
                BackendPlayerHost { service.downgrade(), launcher.clone(), shutdown.clone() }),
            icon_theme_path.clone());
        if (tray.is_ok())
            integrations.add(Box<dyn<PlayerIntegration>>::make(rstd::move(*tray)));
        else
            rstd::log::warn("tray unavailable: {}", tray.unwrap_err());
        if (! integrations.is_empty()) {
            auto attached = co_await service->attach_player_integration(
                Box<dyn<PlayerIntegration>>::make(rstd::move(integrations)));
            if (attached.is_err())
                rstd::log::warn("desktop integration unavailable: {}", attached.unwrap_err());
        }
        auto published = co_await daemon.get()->publish(
            ready.port,
            make_rc<dyn<DaemonHost>>(
                BackendPlayerHost { service.downgrade(), launcher.clone(), shutdown.clone() }));
        if (published.is_err()) {
            rstd::log::error("{}", published.unwrap_err());
            co_return Err(
                rstd::io::Error::from_kind(rstd::io::ErrorKind { rstd::io::ErrorKind::Other }));
        }
        co_return Ok(empty {});
    }
    auto shutdown() -> coro<empty> { return daemon.get()->shutdown(); }
    auto notified() -> coro<empty> {
        co_await daemon->notified();
        co_return empty {};
    }
#else
    startup::Instance& instance;
    auto publish(Arc<QcmService>, Arc<UiLauncher>, liserveto::Shutdown, const startup::Ready& ready)
        -> coro<rstd::io::Result<empty>> {
        co_return instance.publish(ready);
    }
    auto shutdown() -> coro<empty> { co_return empty {}; }
    auto notified() -> coro<empty> {
        rstd::async::watch::Sender<bool> never { false };
        auto                             receiver = never.subscribe();
        (void)co_await receiver.changed();
        co_return empty {};
    }
#endif
};

struct DesktopShutdown {
    rstd::async::Runtime& runtime;
    DesktopSystem&        desktop;
    ~DesktopShutdown() { runtime.block_on(desktop.shutdown()); }
};

auto serve_player_backend(TcpListener listener, Arc<QcmService> service,
                          liserveto::Shutdown stopped, rstd::async::watch::Sender<bool> finished,
                          Arc<SessionRegistry> sessions) -> coro<rstd::io::Result<empty>> {
    auto result = co_await serve_qcm(rstd::move(listener),
                                     rstd::move(service),
                                     rstd::move(stopped),
                                     {},
                                     Some(rstd::move(sessions)));
    (void)finished.send_replace(true);
    (void)rstd_co_try(rstd::move(result));
    co_return Ok(empty {});
}

auto activate_ui(const startup::Instance& instance, const PathBuf& program,
                 const startup::Options& options, StartupTrace& trace)
    -> coro<rstd::io::Result<int>> {
    auto ready = rstd_co_try(
        co_await trace.measure_async(StartupStage::InstanceWait, startup::wait_ready(instance)));
    if (options.no_ui) {
        trace.finish("reused"_str);
        co_return Ok(0);
    }
    auto supervisor = rstd_co_try(rstd::async::ProcessSupervisor::make());
    auto command    = startup::ui_command(program.as_path(), ready, options.ui_args.as_slice());
    auto child      = trace.measure(StartupStage::UiSpawn, [&] {
        return supervisor.handle().spawn(command);
    });
    if (child.is_err()) {
        (void)co_await supervisor.shutdown();
        co_return Err(rstd::move(child).unwrap_err());
    }
    trace.finish("activated"_str);
    auto status = co_await child->wait();
    auto closed = co_await supervisor.shutdown();
    rstd_co_try(closed);
    auto exited = rstd_co_try(rstd::move(status));
    co_return Ok(exited.code().unwrap_or(i32(1)).to_primitive());
}

auto launch_and_serve(TcpListener listener, Arc<QcmService> service, liserveto::Shutdown shutdown,
                      DesktopSystem& desktop, const PathBuf& program, const PathBuf& player_program,
                      const startup::Options& options, const startup::Ready& ready,
                      String activation, StartupTrace& trace) -> coro<rstd::io::Result<empty>> {
    auto supervisor = rstd_co_try(rstd::async::ProcessSupervisor::make());
    auto sessions   = Arc<SessionRegistry>::make();
    auto launcher   = Arc<UiLauncher>::make(supervisor.handle(),
                                            program.clone(),
                                            startup::Ready { ready.port, ready.cache.clone() },
                                            options.ui_args.clone(),
                                            sessions.clone());
    co_await trace.measure_async(
        StartupStage::Player,
        service->start_player(player_program.clone(),
                              rstd::format("http://127.0.0.1:{}", ready.port)));
    auto                             server_stop = rstd_co_try(liserveto::Shutdown::make());
    rstd::async::watch::Sender<bool> finished { false };
    auto                             complete = finished.subscribe();
    auto server = rstd::async::AbortOnDropHandle<rstd::io::Result<empty>> {
        rstd::async::spawn_local(serve_player_backend(rstd::move(listener),
                                                      service.clone(),
                                                      server_stop.clone(),
                                                      finished.clone(),
                                                      sessions.clone()))
    };
    auto published = co_await trace.measure_async(
        StartupStage::Publish,
        desktop.publish(service.clone(), launcher.clone(), shutdown.clone(), ready));
    if (published.is_err()) {
        co_await desktop.shutdown();
        (void)server_stop.request_stop();
        (void)co_await rstd::move(server);
        co_await launcher->finish();
        co_await service->stop_player();
        (void)co_await supervisor.shutdown();
        co_return Err(rstd::move(published).unwrap_err());
    }
    if (! options.no_ui) {
        auto child = co_await trace.measure_async(StartupStage::UiSpawn,
                                                  launcher->open(rstd::move(activation)));
        if (child.is_err()) {
            co_await desktop.shutdown();
            (void)server_stop.request_stop();
            (void)co_await rstd::move(server);
            co_await launcher->finish();
            co_await service->stop_player();
            (void)co_await supervisor.shutdown();
            rstd::log::error("{}", child.unwrap_err());
            co_return Err(
                rstd::io::Error::from_kind(rstd::io::ErrorKind { rstd::io::ErrorKind::Other }));
        }
    }
    trace.finish("ready"_str);
    rstd::log::info("qcm listening on 127.0.0.1:{}", ready.port);
    (void)co_await rstd::async::select(
        shutdown.notified(),
        rstd::async::select(complete.changed(), desktop.notified().into_future()));
    co_await desktop.shutdown();
    co_await service->stop_player();
    (void)server_stop.request_stop();
    auto joined = co_await rstd::move(server);
    co_await launcher->finish();
    auto closed = co_await supervisor.shutdown();
    if (joined.is_err())
        co_return Err(
            rstd::io::Error::from_kind(rstd::io::ErrorKind { rstd::io::ErrorKind::Other }));
    rstd_co_try(rstd::move(*joined));
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
auto watch_desktop_shutdown(DesktopSystem& desktop, liserveto::Shutdown shutdown) -> coro<empty> {
    co_await desktop.notified();
    if (! shutdown.requested()) rstd::log::error("daemon D-Bus ownership was lost");
    (void)shutdown.request_stop();
    co_return empty {};
}
auto run_backend(RuntimeHandle runtime, TcpListener listener, String main_path, String cache_path,
                 ProviderRegistry registry, String cache_dir, String device_id,
                 liserveto::Shutdown shutdown, rstd::signal::Subscription signals,
                 DesktopSystem& desktop, const PathBuf& program, const PathBuf& player_program,
                 const startup::Options& options, const startup::Ready& ready, String activation,
                 StartupTrace& trace) -> rstd::async::coro<Result<empty, ProcessError>> {
    auto signal_task  = rstd::async::AbortOnDropHandle<empty> { rstd::async::spawn_local(
        watch_shutdown_signal(rstd::move(signals), shutdown.clone())) };
    auto desktop_task = rstd::async::AbortOnDropHandle<empty> { rstd::async::spawn_local(
        watch_desktop_shutdown(desktop, shutdown.clone())) };
    auto database     = rstd_co_try(co_await trace.measure_async(
        StartupStage::MainDatabaseOpen, MainDatabase::open(main_path.clone())));
    auto db           = rstd_co_try(database.handle());
    auto initialized =
        co_await trace.measure_async(StartupStage::MainDatabaseMigrate,
                                     database_run(db.clone(), [](sqlito::Connection& connection) {
                                         return migrate(connection);
                                     }));
    if (initialized.is_err()) {
        (void)co_await database.shutdown();
        co_return Err(ProcessError(rstd::move(initialized).unwrap_err()));
    }
    auto restored = co_await trace.measure_async(StartupStage::NetworkRestore,
                                                 restore_network_settings(db.clone()));
    if (restored.is_err()) {
        (void)co_await database.shutdown();
        co_return Err(rstd::move(restored).unwrap_err());
    }
    auto cache = trace.measure(StartupStage::CacheDatabaseOpen, [&] {
        return sqlito::Executor::open(cache_path.as_str());
    });
    if (cache.is_err()) {
        (void)co_await database.shutdown();
        co_return Err(ProcessError(rstd::move(cache).unwrap_err()));
    }
    auto cache_database    = rstd::move(cache).unwrap();
    auto cache_initialized = co_await trace.measure_async(
        StartupStage::CacheDatabaseMigrate,
        database_run(cache_database.handle(),
                     [](sqlito::Connection& connection) -> Result<empty, sqlito::Error> {
                         rstd_try(connection.execute_batch(
                             "PRAGMA journal_mode=WAL;"
                             "PRAGMA synchronous=NORMAL;PRAGMA busy_timeout=5000;"_str));
                         rstd_try(migrate_cache(connection));
                         return Ok(empty {});
                     }));
    if (cache_initialized.is_err()) {
        (void)co_await cache_database.shutdown();
        (void)co_await database.shutdown();
        co_return Err(ProcessError(rstd::move(cache_initialized).unwrap_err()));
    }
    auto created = co_await trace.measure_async(
        StartupStage::Service,
        QcmService::make(db.clone(),
                         rstd::move(runtime),
                         rstd::move(registry),
                         Some(cache_database.handle()),
                         Some(rstd::move(cache_dir)),
                         device_id.is_empty() ? None() : Some(rstd::move(device_id)),
                         Some(rstd::move(main_path))));
    Result<empty, ProcessError> result = Ok(empty {});
    if (created.is_err()) {
        result = Err(rstd::move(created).unwrap_err());
    } else {
        auto started = co_await trace.measure_async(StartupStage::Resources,
                                                    (*created)->start_resource_cache());
        if (started.is_err()) {
            co_await (*created)->stop_providers();
            result = Err(rstd::move(started).unwrap_err());
        } else {
            auto served = co_await launch_and_serve(rstd::move(listener),
                                                    (*created).clone(),
                                                    rstd::move(shutdown),
                                                    desktop,
                                                    program,
                                                    player_program,
                                                    options,
                                                    ready,
                                                    rstd::move(activation),
                                                    trace);
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
    desktop_task.abort();
    (void)co_await rstd::move(desktop_task);
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
    StartupTrace trace(STARTUP_LABELS);
    auto         argument_span = trace.span(StartupStage::Arguments);
    rstd::env::args_init(argc, argv);
    auto arguments  = rstd::iter::from_iter<Vec<OsString>>(rstd::env::args_os());
    auto executable = arguments.is_empty() ? PathBuf::from("qcm"_str)
                                           : PathBuf::from(arguments[usize()].as_os_str());
    auto parsed     = startup::parse(rstd::move(arguments));
    if (parsed.is_err()) return display(parsed.as_ref().unwrap_err());
    if (parsed->is_Display()) {
        (void)argument_span.finish();
        trace.finish("display"_str);
        return display(parsed->as_Display().request);
    }
    auto options = rstd::move(*parsed).as_Parsed().value;
    (void)argument_span.finish();
    auto activation = startup::take_activation_token();
    auto runtime    = trace.measure(StartupStage::Runtime, [] {
        return RuntimeBuilder::current_thread().enable_all().build();
    });
    if (runtime.is_err()) return fail("cannot create runtime\n"_str);
#if defined(__linux__)
    auto daemon = runtime->block_on(
        trace.measure_async(StartupStage::Instance,
                            DaemonBus::claim(String::make(
                                rstd::ffi::CStr::from_ptr(LITO_PKG_VERSION).to_str().unwrap()))));
    if (daemon.is_err()) return fail(rstd::format("{}\n", daemon.unwrap_err()).as_str());
    if (! (*daemon)->primary()) {
        auto notified =
            options.no_ui
                ? Result<empty, String>(Ok(empty {}))
                : runtime->block_on((*daemon).get()->forward_open_ui(rstd::move(activation)));
        runtime->block_on((*daemon).get()->shutdown());
        if (notified.is_err()) return fail(rstd::format("{}\n", notified.unwrap_err()).as_str());
        trace.finish("reused"_str);
        return 0;
    }
    DesktopSystem   desktop { rstd::move(*daemon) };
    DesktopShutdown desktop_shutdown { *runtime, desktop };
#endif
    auto path_span = trace.span(StartupStage::Paths);
    auto paths     = startup::storage_paths(options);
    if (paths.is_err()) return fail(rstd::format("{}\n", paths.as_ref().unwrap_err()).as_str());
    if (rstd::fs::create_dir_all(paths->data.as_path()).is_err())
        return fail("cannot create data directory\n"_str);
    if (rstd::fs::create_dir_all(paths->cache.as_path()).is_err())
        return fail("cannot create cache directory\n"_str);
    auto data_dir   = rstd::fs::canonicalize(paths->data.as_path());
    auto cache_root = rstd::fs::canonicalize(paths->cache.as_path());
    if (data_dir.is_err() || cache_root.is_err())
        return fail("cannot resolve storage directories\n"_str);
    (void)path_span.finish();
    auto instance = trace.measure(StartupStage::Instance, [&] {
        return startup::Instance::open(data_dir->as_path());
    });
    if (instance.is_err())
        return fail(
            rstd::format("cannot lock backend: {}\n", instance.as_ref().unwrap_err()).as_str());
#ifdef __linux__
    auto resolved = rstd::fs::read_link(PathBuf::from("/proc/self/exe"_str).as_path());
    if (resolved.is_ok()) executable = rstd::move(*resolved);
    desktop.icon_theme_path = tray_icon_theme_path(executable.as_path());
#endif
    auto program        = options.ui.is_some() ? PathBuf::from(options.ui->as_str())
                                               : startup::ui_program(executable.as_path());
    auto player_program = options.player.is_some() ? PathBuf::from(options.player->as_str())
                                                   : startup::player_program(executable.as_path());
    if (! instance->primary()) {
#if defined(__linux__)
        return fail("backend data directory is in use by another session\n"_str);
#else
        auto activated = runtime->block_on(activate_ui(*instance, program, options, trace));
        if (activated.is_err())
            return fail(
                rstd::format("cannot activate qcm-ui: {}\n", activated.as_ref().unwrap_err())
                    .as_str());
        return *activated;
#endif
    }
#if ! defined(__linux__)
    DesktopSystem   desktop { *instance };
    DesktopShutdown desktop_shutdown { *runtime, desktop };
#endif
    auto cache_dir = cache_root->join("QcmBackend"_str);
    if (rstd::fs::create_dir_all(cache_dir.as_path()).is_err())
        return fail("cannot create resource cache directory\n"_str);
    auto main_path  = data_dir->join("backend.2.db"_str);
    auto cache_path = data_dir->join("backend_cache.db"_str);
    auto bind       = trace.measure(StartupStage::Listener, [&] {
        return TcpListener::bind(SocketAddr::ipv4_loopback(options.port));
    });
    if (bind.is_err()) return fail("cannot bind backend port\n"_str);
    auto listener = rstd::move(*bind);
    auto address  = listener.local_addr();
    if (address.is_err()) return fail("cannot read backend address\n"_str);
    startup::Ready ready { address->port(),
                           String::make(cache_root->as_path().to_string_lossy().as_str()) };
    auto           setting = trace.measure(StartupStage::Settings, [&] {
        return settings::Setting::load(data_dir->as_path());
    });
    if (setting.is_err()) return fail(rstd::format("{}\n", setting.as_ref().unwrap_err()).as_str());
    auto device_id = rstd::move(setting->device_id);

    auto registry = trace.measure(StartupStage::Plugins, [] {
        return LuaProviderPlugin::load_default();
    });
    if (registry.is_err()) {
        rstd::log::error("cannot load provider registry: {}", registry.as_ref().unwrap_err());
        return fail("cannot load provider registry\n"_str);
    }
    auto shutdown = trace.measure(StartupStage::Shutdown, [] {
        return liserveto::Shutdown::make();
    });
    if (shutdown.is_err()) return fail("cannot create shutdown handle\n"_str);
    Vec<rstd::signal::SignalKind> kinds;
    kinds.push(rstd::signal::SignalKind::interrupt());
#ifndef _WIN32
    auto terminate = rstd::signal::unix::terminate();
    if (terminate.is_err()) return fail("cannot select shutdown signal\n"_str);
    kinds.push(rstd::move(*terminate));
#endif
    auto signals = trace.measure(StartupStage::Signals, [&] {
        return rstd::signal::subscribe(kinds.as_slice());
    });
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
                                      desktop,
                                      program,
                                      player_program,
                                      options,
                                      ready,
                                      rstd::move(activation),
                                      trace));
    if (result.is_err()) {
        rstd::log::error("backend failed: {}", result.as_ref().unwrap_err());
        return fail("backend server failed\n"_str);
    }
    return 0;
}
