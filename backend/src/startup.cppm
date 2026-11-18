export module qcm.backend:startup;
import rstd;
import rstd.argparse;
import rstd.serde;
using rstd::argparse::ErrorReport;
using rstd::argparse::ParseOutcome;
using rstd::ffi::OsString;
using rstd::path::Path;
using rstd::path::PathBuf;
using namespace rstd::prelude;

export namespace qcm::backend::startup
{
struct Options {
    Option<String> data, cache, ui, player;
    u16            port {};
    bool           no_ui {};
    Vec<String>    ui_args;
};

auto take_activation_token() -> String;

auto parse(Vec<OsString> arguments) -> Result<ParseOutcome<Options>, ErrorReport>;

struct Paths {
    PathBuf data, cache;
};

auto storage_paths(const Options& options, Option<ref<Path>> data_root,
                   Option<ref<Path>> cache_root) -> Result<Paths, String>;
auto storage_paths(const Options& options) -> Result<Paths, String>;

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
    ~Instance();

    static auto open(ref<Path> directory) -> rstd::io::Result<Instance>;

    auto primary() const -> bool;

    auto publish(const Ready& ready) -> rstd::io::Result<empty>;

    auto ready() const -> rstd::io::Result<Option<Ready>>;
};

auto wait_ready(const Instance& instance) -> rstd::async::coro<rstd::io::Result<Ready>>;

auto companion_program(ref<Path> executable, ref<str> name) -> PathBuf;

auto ui_program(ref<Path> executable) -> PathBuf;

auto player_program(ref<Path> executable) -> PathBuf;

auto ui_command(ref<Path> program, const Ready& ready, slice<String> arguments)
    -> rstd::process::Command;
} // namespace qcm::backend::startup
