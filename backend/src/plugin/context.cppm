module;
#include <rstd/macro.hpp>
export module qcm.backend:plugin.context;
export import rstd;
export import :plugin.state;
import :uuid;
using rstd::sync::Arc;
using rstd::sync::OnceLock;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct PlatformInfo {
    ref<str> os;
    ref<str> arch;
    ref<str> family;
    ref<str> source;

    static auto current() -> PlatformInfo {
#if defined(__ANDROID__)
        auto os = "android"_str;
#elif defined(_WIN32)
        auto os = "windows"_str;
#elif defined(__linux__)
        auto os = "linux"_str;
#elif __is_target_os(ios)
        auto os = "ios"_str;
#elif defined(RSTD_OS_MACOS)
        auto os = "macos"_str;
#elif defined(__FreeBSD__)
        auto os = "freebsd"_str;
#elif defined(__wasi__)
        auto os = "wasi"_str;
#else
        auto os = "unknown"_str;
#endif
#if defined(__x86_64__) || defined(_M_X64)
        auto arch = "x86_64"_str;
#elif defined(__aarch64__) || defined(_M_ARM64)
        auto arch = "aarch64"_str;
#elif defined(__i386__) || defined(_M_IX86)
        auto arch = "x86"_str;
#elif defined(__arm__) || defined(_M_ARM)
        auto arch = "arm"_str;
#elif defined(__wasm32__)
        auto arch = "wasm32"_str;
#elif defined(__wasm64__)
        auto arch = "wasm64"_str;
#elif defined(__riscv) && __riscv_xlen == 64
        auto arch = "riscv64"_str;
#else
        auto arch = "unknown"_str;
#endif
#if defined(RSTD_OS_WINDOWS)
        auto family = "windows"_str;
#elif defined(RSTD_ARCH_WASM)
        auto family = "wasm"_str;
#elif defined(RSTD_OS_UNIX)
        auto family = "unix"_str;
#else
        auto family = "unknown"_str;
#endif
        return { os, arch, family, "cpp_target"_str };
    }
};
class PluginRuntime {
    String runtime_id_;

public:
    explicit PluginRuntime(String runtime_id): runtime_id_(rstd::move(runtime_id)) {}
    static auto current() -> Result<Arc<PluginRuntime>, rstd::io::Error> {
        static OnceLock<Arc<PluginRuntime>> runtime;
        if (runtime.get().is_none()) {
            auto value = Arc<PluginRuntime>::make(rstd_try(random_uuid()));
            (void)runtime.set(rstd::move(value));
        }
        return Ok((*runtime.get().unwrap()).clone());
    }
    auto runtime_id() const -> ref<str> { return runtime_id_.as_str(); }
    auto platform() const -> PlatformInfo { return PlatformInfo::current(); }
};

class PluginContext {
    PluginState        state_;
    Arc<PluginRuntime> runtime_;
    PlatformInfo       platform_;

    PluginContext(PluginState state, Arc<PluginRuntime> runtime, PlatformInfo platform)
        : state_(rstd::move(state)), runtime_(rstd::move(runtime)), platform_(platform) {}

public:
    static auto create(sqlito::ExecutorHandle db, String ns, String installation,
                       Arc<PluginRuntime> runtime) -> StateResult<PluginContext> {
        auto state =
            rstd_try(PluginState::create(rstd::move(db), rstd::move(ns), rstd::move(installation)));
        auto platform = runtime->platform();
        return Ok(PluginContext(rstd::move(state), rstd::move(runtime), platform));
    }
    auto clone() const -> PluginContext {
        return PluginContext(state_.clone(), runtime_.clone(), platform_);
    }
    auto namespace_name() const -> ref<str> { return state_.namespace_name(); }
    auto installation_id() const -> ref<str> { return state_.installation_id(); }
    auto runtime_id() const -> ref<str> { return runtime_->runtime_id(); }
    auto platform() const -> const PlatformInfo& { return platform_; }
    auto load_state() const -> rstd::async::coro<StateResult<InstallationState>> {
        return state_.load();
    }
    auto compare_exchange_state(u64 revision, String data) const
        -> rstd::async::coro<StateResult<StateUpdate>> {
        return state_.compare_exchange(revision, rstd::move(data));
    }
};
} // namespace qcm::backend
