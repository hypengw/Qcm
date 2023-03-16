#pragma once
#include <string_view>
#include <source_location>

#include "core/fmt.h"

namespace qcm
{
enum class LogLevel
{
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
};

bool handle_assert(std::string_view func, std::string_view file, int line, std::string_view expr,
                   std::string_view reason);
void crash();

constexpr const char* past_last_slash(const char* const path, const int pos = 0,
                                      const int last_slash = 0) {
    if (path[pos] == '\0') return path + last_slash;
    if (path[pos] == '/')
        return past_last_slash(path, pos + 1, pos + 1);
    else
        return past_last_slash(path, pos + 1, last_slash);
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
        log_impl(level, loc, fmt::vformat(fmt, fmt::make_format_args(args...)));
    }

    void set_level(LogLevel);

private:
    void     log_impl(LogLevel level, const std::source_location loc, std::string_view);
    LogLevel m_level;
};
} // namespace qcm

#define __FILE_SHORT__ qcm::past_last_slash(__FILE__)

// clang-format off

#define GENERIC_LOG(lv, ...) qcm::LogManager::instance()->log(lv, std::source_location::current(), __VA_ARGS__);

#define ERROR_LOG(...)   do { GENERIC_LOG(qcm::LogLevel::ERROR,   __VA_ARGS__) } while (false)
#define WARN_LOG(...)    do { GENERIC_LOG(qcm::LogLevel::WARN, __VA_ARGS__) } while (false)
#define INFO_LOG(...)    do { GENERIC_LOG(qcm::LogLevel::INFO,    __VA_ARGS__) } while (false)
#define DEBUG_LOG(...)   do { GENERIC_LOG(qcm::LogLevel::DEBUG,   __VA_ARGS__) } while (false)

// clang-format on

#define _assert_(_a_)                                                                             \
    if (! (_a_)) {                                                                                \
        if (! qcm::handle_assert(__FUNCTION__, __FILE_SHORT__, __LINE__, #_a_, {})) qcm::crash(); \
    }

#define _assert_msg_(_a_, ...)                                                           \
    if (! (_a_)) {                                                                       \
        if (! qcm::handle_assert(                                                        \
                __FUNCTION__, __FILE_SHORT__, __LINE__, #_a_, fmt::format(__VA_ARGS__))) \
            qcm::crash();                                                                \
    }
