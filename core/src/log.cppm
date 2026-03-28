module;
#include <string_view>
#include <string>
#include <source_location>
#include <format>

#define LOG_FUNC(name, NAME)                                                         \
    export template<typename... T>                                                   \
    struct name {                                                                    \
        name(rstd::fmt::format_string<T...> fmt, T&&... args,                        \
             const std::source_location     loc = std::source_location::current()) { \
            if (! log_check(LogLevel::NAME)) return;                                 \
            auto msg = rstd::format(fmt, rstd::forward<T>(args)...);                 \
            log_loc_raw(LogLevel::NAME, loc, { msg.data(), msg.size() });            \
        }                                                                            \
    };                                                                               \
    export template<typename... T>                                                   \
    name(rstd::fmt::format_string<T...>, T&&...) -> name<T...>;

export module qcm.log;
export import qcm.core;

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
export auto log_format(LogLevel level, std::string_view content, std::string_view filename,
                       std::uint_least32_t line, std::uint_least32_t column) -> std::string;
export void log_raw(LogLevel level, std::string_view);
export bool log_check(LogLevel level) noexcept;

export template<typename... T>
void log(LogLevel level, const std::source_location loc, rstd::fmt::format_string<T...> fmt,
         T&&... args) {
    if (! log_check(level)) return;
    auto msg = rstd::format(fmt, rstd::forward<T>(args)...);
    log_loc_raw(level, loc, { msg.data(), msg.size() });
}

LOG_FUNC(debug, DEBUG);
LOG_FUNC(info, INFO);
LOG_FUNC(warn, WARN);
LOG_FUNC(error, ERROR);

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