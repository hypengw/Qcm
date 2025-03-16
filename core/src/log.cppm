module;
#include <string_view>
#include <string>
#include <source_location>
#include <format>
export module qcm.log;

namespace qcm
{

export enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
};
export struct LogManager {
    static LogManager* init();
    static LogManager* instance();

    virtual auto level() const -> LogLevel = 0;
    virtual void set_level(LogLevel)       = 0;
};

namespace log
{

export auto level_from(std::string_view) -> LogLevel;

export void log_loc_raw(LogLevel level, const std::source_location loc, std::string_view);
export void log_raw(LogLevel level, std::string_view);

export template<typename... T>
void log(LogLevel level, const std::source_location loc, std::format_string<T...> fmt,
         T&&... args) {
    if (level < LogManager::instance()->level()) return;
    log_loc_raw(level, loc, std::vformat(fmt.get(), std::make_format_args(args...)));
}

export template<typename... T>
struct error {
    error(std::format_string<T...>   fmt, T&&... args,
          const std::source_location loc = std::source_location::current()) {
        if (LogLevel::ERROR < LogManager::instance()->level()) return;
        log_loc_raw(LogLevel::ERROR, loc, std::vformat(fmt.get(), std::make_format_args(args...)));
    }
};
export template<typename... T>
error(std::format_string<T...>, T&&...) -> error<T...>;

export template<typename... T>
struct warn {
    warn(std::format_string<T...>   fmt, T&&... args,
         const std::source_location loc = std::source_location::current()) {
        if (LogLevel::WARN < LogManager::instance()->level()) return;
        log_loc_raw(LogLevel::WARN, loc, std::vformat(fmt.get(), std::make_format_args(args...)));
    }
};
export template<typename... T>
warn(std::format_string<T...>, T&&...) -> warn<T...>;

export template<typename... T>
struct info {
    info(std::format_string<T...>   fmt, T&&... args,
         const std::source_location loc = std::source_location::current()) {
        if (LogLevel::INFO < LogManager::instance()->level()) return;
        log_loc_raw(LogLevel::INFO, loc, std::vformat(fmt.get(), std::make_format_args(args...)));
    }
};
export template<typename... T>
info(std::format_string<T...>, T&&...) -> info<T...>;

export template<typename... T>
struct debug {
    debug(std::format_string<T...>   fmt, T&&... args,
          const std::source_location loc = std::source_location::current()) {
        if (LogLevel::DEBUG < LogManager::instance()->level()) return;
        log_loc_raw(LogLevel::DEBUG, loc, std::vformat(fmt.get(), std::make_format_args(args...)));
    }
};
export template<typename... T>
debug(std::format_string<T...>, T&&...) -> debug<T...>;

export inline constexpr void             noop() {}
export inline constexpr std::string_view noop_str() { return {}; }

[[noreturn]] inline void fail(std::string_view msg) {
    log_raw(LogLevel::ERROR, msg);

    // Crash with access violation and generate crash report.
    volatile auto nullptr_value = (int*)nullptr;
    *nullptr_value              = 0;

    // Silent the possible failure to comply noreturn warning.
    std::abort();
}

export constexpr auto enable_debug {
#ifdef NDEBUG
    false
#else
    true
#endif
};

export std::string format_assert(std::string_view expr_str, const std::source_location& loc,
                                 std::string_view msg);

export template<bool Enabled, typename Expr, typename Msg>
    requires Enabled
void handle_assert(Expr&& expr, std::string_view expr_str, Msg&& msg,
                   const std::source_location loc = std::source_location::current()) {
    if (! expr()) {
        fail(format_assert(expr_str, loc, msg()));
    }
}

export template<bool Enabled, typename Expr, typename Msg>
    requires(! Enabled)
void handle_assert(Expr&&, std::string_view, Msg&&,
                   const std::source_location = std::source_location::current()) {}

} // namespace log

} // namespace qcm