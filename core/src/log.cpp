module;
#include <chrono>
#include <format>
#include <iterator>
#include <cassert>
#include <cstdio>
#include <source_location>

module qcm.log;
import qcm.helper;
import platform;

namespace
{

constexpr const char* past_last_slash(const char* const path, const int pos = 0,
                                      const int last_slash = 0) {
    if (path[pos] == '\0') return path + last_slash;
    if (path[pos] == '/' || path[pos] == '\\')
        return past_last_slash(path, pos + 1, pos + 1);
    else
        return past_last_slash(path, pos + 1, last_slash);
}

constexpr auto extract_last(std::string_view path, std::size_t count) -> std::string_view {
    auto size = path.size();
    while (size != 0) {
        if (path[size - 1] == '/' || path[size - 1] == '\\') {
            --count;
        }
        if (count != 0) {
            --size;
        } else {
            break;
        }
    }
    return std::string_view { path.begin() + size, path.end() };
}

constexpr auto extract_basename(std::string_view path) -> std::string_view {
    return extract_last(path, 1);
}

using LogLevel = qcm::LogLevel;

constexpr auto COLOR_RESET   = "\033[0m"sv;
constexpr auto COLOR_RED     = "\033[31m"sv;
constexpr auto COLOR_GREEN   = "\033[32m"sv;
constexpr auto COLOR_YELLOW  = "\033[33m"sv;
constexpr auto COLOR_BLUE    = "\033[34m"sv;
constexpr auto COLOR_MAGENTA = "\033[35m"sv;
constexpr auto COLOR_CYAN    = "\033[36m"sv;
constexpr auto COLOR_WHITE   = "\033[37m"sv;

auto get_log_color(LogLevel level) -> std::string_view {
    switch (level) {
    case LogLevel::INFO: return COLOR_GREEN;
    case LogLevel::WARN: return COLOR_YELLOW;
    case LogLevel::ERROR: return COLOR_RED;
    case LogLevel::DEBUG: return COLOR_BLUE;
    default: return COLOR_RESET;
    }
}

std::string_view to_sv(qcm::LogLevel lv) {
#define X(E) \
    case E: return #E

    switch (lv) {
        using enum qcm::LogLevel;
        X(DEBUG);
        X(INFO);
        X(WARN);
        X(ERROR);
    default: return "UNKNOWN";
    }
#undef X
}

bool check_stderr(qcm::LogLevel lv) {
    switch (lv) {
        using enum qcm::LogLevel;
    case ERROR:
    case WARN: return true;
    default: return false;
    }
}

struct file_iterator {
    FILE* file;

    using iterator_category = std::output_iterator_tag;
    using value_type        = void;
    using difference_type   = std::ptrdiff_t;
    using pointer           = void;
    using reference         = void;

    file_iterator& operator=(char c) {
        fputc(c, file);
        return *this;
    }

    file_iterator& operator*() { return *this; }
    file_iterator& operator++() { return *this; }
    file_iterator  operator++(int) { return *this; }
};

} // namespace

auto the_log_manager() -> qcm::LogManager*;

namespace qcm
{

LogManager* LogManager::init() { return the_log_manager(); }

LogManager* LogManager::instance() { return the_log_manager(); }
void        log::log_raw(LogLevel level, std::string_view content) {
    FILE* out = nullptr;
    switch (level) {
        using enum LogLevel;
    case DEBUG:
    case INFO: out = stdout; break;
    case WARN:
    case ERROR: out = stderr; break;
    }
    if (out == nullptr) return;

    file_iterator out_iter(out);
    if (plt::support_color()) {
        std::format_to(out_iter, "{}{}{}", get_log_color(level), content, COLOR_RESET);
    } else {
        std::format_to(out_iter, "{}", content);
    }
    std::fflush(out);
};
void log::log_loc_raw(LogLevel level, const std::source_location loc, std::string_view content) {
    using namespace std::chrono;
#ifdef _LIBCPP_VERSION
    auto time_str = "";
#else
    auto now_utc   = system_clock::now();
    auto now_local = std::chrono::current_zone()->to_local(now_utc);
    auto time_str  = std::format("{:%Y-%m-%dT%H:%M:%S}", now_local);
#endif
    log_raw(level,
            std::format("{:.26s}Z {:>5} {}({},{}): {}  \n",
                        time_str,
                        to_sv(level),
                        extract_last(loc.file_name(), 2),
                        loc.line(),
                        loc.column(),
                        content));
}

auto log::format_assert(std::string_view expr_str, const std::source_location& loc,
                        std::string_view msg) -> std::string {
    return std::format("{}:{}: {}: Assertion `{}` failed.{}{}\n",
                       extract_last(loc.file_name(), 2),
                       loc.line(),
                       loc.function_name(),
                       expr_str,
                       msg.empty() ? "" : "\n",
                       msg);
}

auto log::level_from(std::string_view in) -> LogLevel {
    if (in == "debug")
        return LogLevel::DEBUG;
    else if (in == "info")
        return LogLevel::INFO;
    else if (in == "warn")
        return LogLevel::WARN;
    else if (in == "error")
        return LogLevel::ERROR;
    else
        return LogLevel::WARN;
}
} // namespace qcm
