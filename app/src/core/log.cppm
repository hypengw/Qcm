module;
#include <cstdint>
#include <string_view>
#include <string>
#include <source_location>

export module qcm.log;
export import qcm.core;
import rstd;

export namespace qcm
{
enum class LogLevel
{
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
};

struct LogManager {
    static LogManager* instance();
    virtual auto       level() const -> LogLevel = 0;
    virtual void       set_level(LogLevel)       = 0;
};

namespace log
{
auto level_from(std::string_view) -> LogLevel;
void log_loc_raw(LogLevel level, const std::source_location loc, std::string_view);
auto log_format(LogLevel level, std::string_view content, std::string_view filename,
                std::uint_least32_t line, std::uint_least32_t column) -> std::string;
void log_raw(LogLevel level, std::string_view);
bool log_check(LogLevel level) noexcept;

template<typename... T>
void log(LogLevel level, const std::source_location loc, rstd::fmt::format_string<T...> fmt,
         T&&... args) {
    if (! log_check(level)) return;
    auto msg = rstd::format(fmt, rstd::forward<T>(args)...);
    log_loc_raw(level,
                loc,
                { reinterpret_cast<const char*>(msg.as_str().data()), msg.size().to_primitive() });
}
} // namespace log
} // namespace qcm
