module;
#include <rstd/macro.hpp>
export module qcm.backend:startup;
export import rstd;
export import rstd.argparse;
import rstd.json;
using rstd::argparse::Arg;
using rstd::argparse::Command;
using rstd::argparse::ErrorReport;
using rstd::argparse::NumArgs;
using rstd::argparse::ParseOutcome;
using rstd::argparse::ValueError;
using rstd::ffi::OsStr;
using rstd::ffi::OsString;
using rstd::path::Path;
using rstd::path::PathBuf;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::startup
{
struct Options {
    Option<String> data, cache, ui, player;
    u16            port {};
    bool           no_ui {};
    Vec<String>    ui_args;
};

auto parse(Vec<OsString> arguments) -> Result<ParseOutcome<Options>, ErrorReport> {
    auto command = Command::make("qcm"_str);
    command.about("Start the persistent backend and open qcm-ui after the socket is ready."_str);
    command.after_help("Run qcm again to open the UI using the existing backend.\n"
                       "Pass UI options after --, for example: qcm -- --log-level debug"_str);
    auto path_parser =
        rstd::argparse::parse_with<String>([](ref<OsStr> value) -> Result<String, ValueError> {
            auto text = rstd_try(rstd::argparse::string_parser().parse(value));
            if (text.is_empty()) return Err(ValueError::Message("path must not be empty"_Str));
            return Ok(rstd::move(text));
        });
    auto data  = command.add_arg(Arg<String>::value("data"_str, path_parser)
                                     .short_name(u8('d'))
                                     .long_name("data"_str)
                                     .value_name("DIR"_str)
                                     .help("Backend data directory"_str));
    auto cache = command.add_arg(Arg<String>::value("cache"_str, path_parser)
                                     .short_name(u8('c'))
                                     .long_name("cache"_str)
                                     .value_name("DIR"_str)
                                     .help("Cache directory"_str));
    auto port  = command.add_arg(Arg<u16>::value("port"_str, rstd::argparse::from_str_parser<u16>())
                                     .short_name(u8('p'))
                                     .long_name("port"_str)
                                     .value_name("PORT"_str)
                                     .help("Listen port; 0 selects an available port"_str));
    auto ui    = command.add_arg(Arg<String>::value("ui"_str, path_parser)
                                     .long_name("ui"_str)
                                     .value_name("PATH"_str)
                                     .help("UI executable"_str));
    auto player = command.add_arg(Arg<String>::value("player"_str, path_parser)
                                      .long_name("player"_str)
                                      .value_name("PATH"_str)
                                      .help("Audio player executable"_str));
    auto no_ui  = command.add_arg(Arg<bool>::flag("no-ui"_str)
                                      .long_name("no-ui"_str)
                                      .help("Run the backend without opening the UI"_str));
    auto ui_args =
        command.add_arg(Arg<String>::value("ui-args"_str, rstd::argparse::string_parser())
                            .num_args(NumArgs::any())
                            .value_name("UI_ARGS"_str)
                            .help("Arguments forwarded to qcm-ui"_str));
    auto parser = rstd::move(command).build().unwrap();
    auto parsed = parser.parse_from(rstd::move(arguments));
    if (parsed.is_err()) return Err(parser.render_error(parsed.as_ref().unwrap_err()));
    if (parsed->is_Display())
        return Ok(ParseOutcome<Options>::Display(rstd::move(*parsed).as_Display().request));
    auto    matches = rstd::move(*parsed).as_Parsed().value;
    Options options;
    options.data   = matches.get_one(data).unwrap().map([](ref<String> value) {
        return value->clone();
    });
    options.cache  = matches.get_one(cache).unwrap().map([](ref<String> value) {
        return value->clone();
    });
    options.ui     = matches.get_one(ui).unwrap().map([](ref<String> value) {
        return value->clone();
    });
    options.player = matches.get_one(player).unwrap().map([](ref<String> value) {
        return value->clone();
    });
    if (auto value = matches.get_one(port).unwrap(); value.is_some()) options.port = **value;
    if (auto value = matches.get_one(no_ui).unwrap(); value.is_some()) options.no_ui = **value;
    if (auto values = matches.get_many(ui_args).unwrap(); values.is_some()) {
        for (auto value : *values) options.ui_args.push(value->clone());
    }
    return Ok(ParseOutcome<Options>::Parsed(rstd::move(options)));
}

struct Paths {
    PathBuf data, cache;
};

auto storage_paths(const Options& options, Option<ref<Path>> data_root,
                   Option<ref<Path>> cache_root) -> Result<Paths, String> {
    Option<PathBuf> data, cache;
    if (options.data.is_some()) data = Some(PathBuf::from(options.data->as_str()));
    if (options.cache.is_some()) cache = Some(PathBuf::from(options.cache->as_str()));
    if (data.is_none() && data_root.is_some())
        data = Some(PathBuf::from(*data_root).join("Qcm/Qcm"_str));
    if (cache.is_none() && cache_root.is_some()) {
        cache = Some(PathBuf::from(*cache_root).join("Qcm/Qcm"_str));
#ifdef _WIN32
        cache->push("cache"_str);
#endif
    }
    if (data.is_none()) return Err("default data directory is unavailable; specify --data"_Str);
    if (cache.is_none()) return Err("default cache directory is unavailable; specify --cache"_Str);
    return Ok(Paths { rstd::move(*data), rstd::move(*cache) });
}

auto storage_paths(const Options& options) -> Result<Paths, String> {
    if (options.data.is_some() && options.cache.is_some())
        return storage_paths(options, None(), None());
    auto base = rstd_try(rstd::dirs::BaseDirs::detect(), [](auto error) {
        return rstd::format("cannot detect default storage directories: {}", error);
    });
    return storage_paths(options, base.data_dir(), base.cache_dir());
}

struct [[pmacro::derive("serde::derive")]] Ready {
    u16    port;
    String cache;
};

class Instance {
    PathBuf                    record_;
    Option<rstd::fs::FileLock> lock_;
    Instance(PathBuf record, Option<rstd::fs::FileLock> lock)
        : record_(rstd::move(record)), lock_(rstd::move(lock)) {}

public:
    Instance(const Instance&) = delete;
    Instance(Instance&& other) noexcept
        : record_(rstd::move(other.record_)), lock_(other.lock_.take()) {}
    auto operator=(Instance&&) -> Instance& = delete;
    ~Instance() {
        if (lock_.is_some()) (void)rstd::fs::remove_file(record_.as_path());
    }

    static auto open(ref<Path> directory) -> rstd::io::Result<Instance> {
        auto path = PathBuf::from(directory).join("qcm.lock"_str);
        auto file = rstd_try(
            rstd::fs::File::options().read(true).write(true).create(true).mode(u32(0600)).open(
                path.as_path()));
        auto lock = rstd_try(
            rstd::fs::FileLock::try_acquire(rstd::move(file), rstd::fs::FileLockMode::Exclusive));
        auto record = PathBuf::from(directory).join("qcm.ready.json"_str);
        if (lock.is_some()) {
            auto removed = rstd::fs::remove_file(record.as_path());
            if (removed.is_err() &&
                removed.as_ref().unwrap_err().kind().code != rstd::io::ErrorKind::NotFound)
                return Err(rstd::move(removed).unwrap_err());
        }
        return Ok(Instance(rstd::move(record), rstd::move(lock)));
    }

    auto primary() const -> bool { return lock_.is_some(); }

    auto publish(const Ready& ready) -> rstd::io::Result<empty> {
        if (! primary() || ready.port == u16() || ready.cache.is_empty())
            return Err(rstd::io::Error::from_kind(
                rstd::io::ErrorKind { rstd::io::ErrorKind::InvalidInput }));
        auto text      = rstd_try(rstd::json::encode(ready), [](auto error) {
            return rstd::io::Error::with_source(
                rstd::io::ErrorKind { rstd::io::ErrorKind::InvalidData },
                Box<dyn<rstd::error::Error>>::make(rstd::move(error)));
        });
        auto temporary = PathBuf::from(record_.as_path());
        temporary.set_extension("json.tmp"_str);
        {
            auto file = rstd_try(rstd::fs::File::options()
                                     .write(true)
                                     .create(true)
                                     .truncate(true)
                                     .mode(u32(0600))
                                     .open(temporary.as_path()));
            rstd_try(file.write_all(text.as_str().as_bytes()));
        }
        return rstd::fs::rename(temporary.as_path(), record_.as_path());
    }

    auto ready() const -> rstd::io::Result<Option<Ready>> {
        auto text = rstd::fs::read_to_string(record_.as_path());
        if (text.is_err()) {
            auto error = rstd::move(text).unwrap_err();
            if (error.kind().code == rstd::io::ErrorKind::NotFound) return Ok(None());
            return Err(rstd::move(error));
        }
        auto decoded = rstd_try(rstd::json::decode<Ready>(text->as_str()), [](auto error) {
            return rstd::io::Error::with_source(
                rstd::io::ErrorKind { rstd::io::ErrorKind::InvalidData },
                Box<dyn<rstd::error::Error>>::make(rstd::move(error)));
        });
        if (decoded.port == u16() || decoded.cache.is_empty())
            return Err(rstd::io::Error::from_kind(
                rstd::io::ErrorKind { rstd::io::ErrorKind::InvalidData }));
        return Ok(Some(rstd::move(decoded)));
    }
};

auto wait_ready(const Instance& instance) -> rstd::async::coro<rstd::io::Result<Ready>> {
    for (unsigned attempt = 0; attempt < 100; ++attempt) {
        auto ready = rstd_co_try(instance.ready());
        if (ready.is_some()) {
            auto connected = co_await rstd::async::select(
                rstd::async::into_future(rstd::net::TcpStream::connect(
                    rstd::net::SocketAddr::ipv4_loopback(ready->port))),
                rstd::async::sleep(Duration::from_millis(u64(200))));
            if (connected.is_left() && rstd::move(connected).unwrap_left().is_ok())
                co_return Ok(rstd::move(*ready));
        }
        co_await rstd::async::sleep(Duration::from_millis(u64(50)));
    }
    co_return Err(
        rstd::io::Error::from_kind(rstd::io::ErrorKind { rstd::io::ErrorKind::TimedOut }));
}

auto companion_program(ref<Path> executable, ref<str> name) -> PathBuf {
    auto parent = executable.parent();
    if (parent.is_some()) {
        auto managed = rstd::format("../../../bin/{}", name);
        for (auto relative : { name, managed.as_str() }) {
            auto candidate = PathBuf::from(*parent).join(relative);
            auto exists    = rstd::fs::exists(candidate.as_path());
            if (exists.is_ok() && *exists) return candidate;
        }
    }
    return PathBuf::from(name);
}

auto ui_program(ref<Path> executable) -> PathBuf {
#ifdef _WIN32
    return companion_program(executable, "qcm-ui.exe"_str);
#else
    return companion_program(executable, "qcm-ui"_str);
#endif
}

auto player_program(ref<Path> executable) -> PathBuf {
#ifdef _WIN32
    return companion_program(executable, "qcm-player.exe"_str);
#else
    return companion_program(executable, "qcm-player"_str);
#endif
}

auto ui_command(ref<Path> program, const Ready& ready, slice<String> arguments)
    -> rstd::process::Command {
    auto command = rstd::process::Command::make(program.as_os_str());
    for (const auto& argument : arguments) command.arg(argument.as_str());
    auto port = rstd::format("{}", ready.port);
    command.arg("--port"_str).arg(port.as_str()).arg("--cache"_str).arg(ready.cache.as_str());
    return command;
}
} // namespace qcm::backend::startup
