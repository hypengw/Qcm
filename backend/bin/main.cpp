#include <rstd/macro.hpp>
import qcm.backend;
import rstd.log;
import rstd;
import sqlito;
using rstd::async::RuntimeBuilder;
using rstd::async::RuntimeHandle;
using rstd::net::SocketAddr;
using rstd::net::TcpListener;
using rstd::path::PathBuf;
using namespace rstd::prelude;
using namespace rstd::literals;
using namespace qcm::backend;

namespace
{
auto arg_str(char* value) -> ref<str> {
    usize len {};
    while (value[len.to_primitive()] != '\0') len = len + usize(1);
    return rstd::str_::from_utf8_unchecked(
        slice<u8>::from_raw_parts(reinterpret_cast<const rstd::byte*>(value), len));
}
auto fail(ref<str> message) -> int {
    auto error = rstd::io::stderr();
    (void)rstd::io::write_all(error, message.as_bytes());
    (void)rstd::as<rstd::io::Write>(error).flush();
    return 1;
}
auto parse_port(ref<str> value) -> Option<u16> {
    if (value.is_empty()) return Option<u16>();
    u32 port {};
    for (auto c : value.as_bytes()) {
        if (c < u8('0') || c > u8('9')) return Option<u16>();
        port = port * u32(10) + u32(c.to_primitive() - '0');
        if (port > u32(65535)) return Option<u16>();
    }
    return Some(u16(static_cast<unsigned short>(port.to_primitive())));
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
    }
    shutdown.request_stop();
    co_return empty {};
}
auto run_backend(RuntimeHandle runtime, TcpListener listener, String main_path, String cache_path,
                 ProviderRegistry registry, String cache_dir, String device_id,
                 liserveto::Shutdown shutdown, rstd::signal::Subscription signals)
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
        auto address = listener.local_addr();
        if (address.is_err()) {
            co_await (*created)->stop_providers();
            result = Err(ProcessError::internal(rstd::move(address).unwrap_err()));
        } else {
            auto started = co_await (*created)->start_resource_cache();
            if (started.is_err()) {
                co_await (*created)->stop_providers();
                result = Err(rstd::move(started).unwrap_err());
            } else {
                auto line = rstd::format("{{\"port\": {}}}\n", address->port().to_primitive());
                auto out  = rstd::io::stdout();
                (void)rstd::io::write_all(out, line.as_str().as_bytes());
                (void)rstd::as<rstd::io::Write>(out).flush();
                auto served = co_await serve_qcm(
                    rstd::move(listener), rstd::move(created).unwrap(), rstd::move(shutdown));
                if (served.is_err())
                    result = Err(ProcessError::internal(rstd::move(served).unwrap_err()));
            }
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
    Option<String> data;
    Option<String> cache;
    Option<u16>    port;
    for (int i = 1; i < argc; ++i) {
        auto arg  = arg_str(argv[i]);
        auto take = [&]() -> Option<ref<str>> {
            if (i + 1 >= argc) return Option<ref<str>>();
            ++i;
            return Some(arg_str(argv[i]));
        };
        if (arg == "--data"_str || arg == "-d"_str) {
            auto value = take();
            if (value.is_none()) return fail("missing --data value\n"_str);
            data = Some(String::make(*value));
        } else if (arg == "--cache"_str || arg == "-c"_str) {
            auto value = take();
            if (value.is_none()) return fail("missing --cache value\n"_str);
            cache = Some(String::make(*value));
        } else if (arg == "--port"_str || arg == "-p"_str) {
            auto value = take();
            if (value.is_none()) return fail("missing --port value\n"_str);
            auto parsed = parse_port(*value);
            if (parsed.is_none()) return fail("invalid --port value\n"_str);
            port = Some(*parsed);
        } else {
            return fail("unknown argument\n"_str);
        }
    }
    if (data.is_none() || cache.is_none()) return fail("--data and --cache are required\n"_str);
    auto data_dir  = PathBuf::from(data->as_str());
    auto cache_dir = PathBuf::from(cache->as_str()).join("QcmBackend"_str);
    if (rstd::fs::create_dir_all(data_dir.as_path()).is_err())
        return fail("cannot create data directory\n"_str);
    if (rstd::fs::create_dir_all(cache_dir.as_path()).is_err())
        return fail("cannot create cache directory\n"_str);

    auto main_path = data->clone();
    main_path.push_str("/backend.2.db"_str);
    auto cache_path = data->clone();
    cache_path.push_str("/backend_cache.db"_str);

    auto bind = TcpListener::bind(SocketAddr::ipv4_loopback(port.is_some() ? *port : u16()));
    if (bind.is_err()) return fail("cannot bind backend port\n"_str);
    auto listener = rstd::move(bind).unwrap();

    auto setting = settings::Setting::load(data_dir.as_path());
    if (setting.is_err()) return fail(rstd::format("{}\n", setting.as_ref().unwrap_err()).as_str());
    auto device_id = rstd::move(setting->device_id);

    static auto logger = rstd::log::EnvLogger {};
    (void)rstd::log::set_logger(logger);
    rstd::log::set_max_level(logger.filter());

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
    auto runtime = RuntimeBuilder::current_thread().enable_all().build();
    if (runtime.is_err()) return fail("cannot create runtime\n"_str);
    auto result =
        runtime->block_on(run_backend(runtime->handle(),
                                      rstd::move(listener),
                                      rstd::move(main_path),
                                      rstd::move(cache_path),
                                      rstd::move(*registry),
                                      String::make(cache_dir.as_path().to_string_lossy().as_str()),
                                      rstd::move(device_id),
                                      rstd::move(shutdown).unwrap(),
                                      rstd::move(signals).unwrap()));
    if (result.is_err()) {
        rstd::log::error("backend failed: {}", result.as_ref().unwrap_err());
        return fail("backend server failed\n"_str);
    }
    return 0;
}
