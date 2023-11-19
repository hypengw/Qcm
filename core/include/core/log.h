#pragma once
#include <string_view>
#include <source_location>

#include "core/fmt.h"

namespace qcm
{
namespace log
{

enum class LogLevel
{
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
};

constexpr const char* past_last_slash(const char* const path, const int pos = 0,
                                      const int last_slash = 0) {
    if (path[pos] == '\0') return path + last_slash;
    if (path[pos] == '/' || path[pos] == '\\')
        return past_last_slash(path, pos + 1, pos + 1);
    else
        return past_last_slash(path, pos + 1, last_slash);
}

constexpr std::string_view extract_basename(std::string_view path) {
    auto size = path.size();
    while (size != 0 && path[size - 1] != '/' && path[size - 1] != '\\') {
        --size;
    }
    return std::string_view { path.begin() + size, path.end() };
}

class LogManager {
public:
    static LogManager* init();
    static LogManager* instance();

    LogManager();
    ~LogManager();

    template<typename... T>
    void log(LogLevel level, const std::source_location loc, fmt::format_string<T...> fmt,
             T&&... args) {
        if (level < m_level) return;
        log_loc_raw(level, loc, fmt::vformat(fmt, fmt::make_format_args(args...)));
    }

    void log_loc_raw(LogLevel level, const std::source_location loc, std::string_view);
    void log_raw(LogLevel level, std::string_view);

    void set_level(LogLevel);

private:
    LogLevel m_level;
};

inline constexpr void             noop() {}
inline constexpr std::string_view noop_str() { return {}; }

[[noreturn]] inline void fail(std::string_view msg) {
    LogManager::instance()->log_raw(LogLevel::ERROR, msg);

    // Crash with access violation and generate crash report.
    volatile auto nullptr_value = (int*)nullptr;
    *nullptr_value              = 0;

    // Silent the possible failure to comply noreturn warning.
    std::abort();
}

constexpr auto enable_debug {
#ifdef NDEBUG
    false
#else
    true
#endif
};

std::string format_assert(std::string_view expr_str, const std::source_location& loc,
                          std::string_view msg);

template<bool Enabled, typename Expr, typename Msg>
    requires Enabled
void handle_assert(Expr&& expr, std::string_view expr_str, Msg&& msg,
                   const std::source_location loc = std::source_location::current()) {
    if (! expr()) {
        fail(format_assert(expr_str, loc, msg()));
    }
}

template<bool Enabled, typename Expr, typename Msg>
    requires(! Enabled)
void handle_assert(Expr&&, std::string_view, Msg&&,
                   const std::source_location = std::source_location::current()) {}

} // namespace log

using LogLevel   = log::LogLevel;
using LogManager = log::LogManager;

} // namespace qcm

// clang-format off
#define GENERIC_LOG(lv, ...) qcm::LogManager::instance()->log(lv, std::source_location::current(), __VA_ARGS__);

#define ERROR_LOG(...)   do { GENERIC_LOG(qcm::LogLevel::ERROR,   __VA_ARGS__) } while (false)
#define WARN_LOG(...)    do { GENERIC_LOG(qcm::LogLevel::WARN, __VA_ARGS__) } while (false)
#define INFO_LOG(...)    do { GENERIC_LOG(qcm::LogLevel::INFO,    __VA_ARGS__) } while (false)
#define DEBUG_LOG(...)   do { GENERIC_LOG(qcm::LogLevel::DEBUG,   __VA_ARGS__) } while (false)
// clang-format on

#define _tpl_assert_(ENABLED, EXPR, LOC) \
    qcm::log::handle_assert<ENABLED>(    \
        [&]() -> bool {                  \
            return (bool)(EXPR);         \
        },                               \
        #EXPR,                           \
        qcm::log::noop_str,              \
        LOC)

#define _tpl_assert_msg_(ENABLED, EXPR, LOC, ...) \
    qcm::log::handle_assert<ENABLED>(            \
        [&]() -> bool {                          \
            return (bool)(EXPR);                 \
        },                                       \
        #EXPR,                                   \
        [&]() -> std::string {                   \
            return fmt::format(__VA_ARGS__);     \
        },                                       \
        LOC)

#define _assert_(EXPR)     _tpl_assert_(qcm::log::enable_debug, EXPR, std::source_location::current())
#define _assert_msg_(EXPR, ...) _tpl_assert_msg_(qcm::log::enable_debug, EXPR, std::source_location::current(), __VA_ARGS__)

#define _assert_rel_(EXPR)     _tpl_assert_(true, EXPR, std::source_location::current())
#define _assert_msg_rel_(EXPR, ...) _tpl_assert_msg_(true, EXPR,std::source_location::current() , __VA_ARGS__)
