#include "core/core.h"
#include "core/log.h"
#include "core/strv_helper.h"
#include <fmt/chrono.h>

#include <cassert>
#include <cstdio>

using namespace qcm;

namespace
{

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
    case E: return "["#E"]"

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

#ifdef _WIN32

auto supports_color() -> bool {
    // Get the handle to the standard output
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Retrieve the current console mode
    DWORD consoleMode = 0;
    if (! GetConsoleMode(hStdout, &consoleMode)) {
        return false;
    }

    // Check if the console supports virtual terminal sequences (ANSI colors)
    return (consoleMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}

#else

auto supports_color() -> bool {
    // Check if the standard output is a terminal
    if (! isatty(STDOUT_FILENO)) {
        return false;
    }

    // Get the TERM environment variable
    auto term = helper::get_env_var("TERM");
    if (! term) {
        return false;
    }

    // Check if the TERM value contains keywords indicating color support
    return term->find("color") != std::string_view::npos ||
           term->find("xterm") != std::string_view::npos ||
           term->find("screen") != std::string_view::npos ||
           term->find("tmux") != std::string_view::npos;
}
#endif

} // namespace

static up<LogManager> log_manager;

LogManager* LogManager::init() {
    log_manager = std::make_unique<LogManager>();
    return LogManager::instance();
}

LogManager* LogManager::instance() { return log_manager.get(); }

LogManager::LogManager(): m_level(LogLevel::WARN) {}
LogManager::~LogManager() {}

void LogManager::set_level(LogLevel l) { m_level = l; }

void LogManager::log_raw(LogLevel level, std::string_view content) {
    FILE* out = nullptr;
    switch (level) {
        using enum LogLevel;
    case DEBUG:
    case INFO: out = stdout; break;
    case WARN:
    case ERROR: out = stderr; break;
    }
    if (supports_color()) {
        fmt::print(out, "{}{}{}", get_log_color(level), content, COLOR_RESET);
    } else {
        fmt::print(out, "{}", content);
    }
    std::fflush(out);
};
void LogManager::log_loc_raw(LogLevel level, const std::source_location loc,
                             std::string_view content) {
    std::time_t t = std::time(nullptr);
    log_raw(level,
            fmt::format("{:<7} [{:%H:%M:%S}] {} [{}:{},{}] \n",
                        to_sv(level),
                        fmt::localtime(t),
                        content,
                        extract_last(loc.file_name(), 2),
                        loc.line(),
                        loc.column()));
}

std::string log::format_assert(std::string_view expr_str, const std::source_location& loc,
                               std::string_view msg) {
    return fmt::format("{}:{}: {}: Assertion `{}` failed.{}{}\n",
                       extract_last(loc.file_name(), 2),
                       loc.line(),
                       loc.function_name(),
                       expr_str,
                       msg.empty() ? "" : "\n",
                       msg);
}