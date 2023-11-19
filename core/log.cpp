#include "core/core.h"
#include "core/log.h"

#include <cassert>
#include <cstdio>

using namespace qcm;

namespace
{

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
    fmt::print(out, "{}", content);
    std::fflush(out);
};
void LogManager::log_loc_raw(LogLevel level, const std::source_location loc,
                             std::string_view content) {
    log_raw(level,
            fmt::format("{} {} at {}({}:{})\n",
                        to_sv(level),
                        content,
                        loc.file_name(),
                        loc.line(),
                        loc.column()));
}

std::string log::format_assert(std::string_view expr_str, const std::source_location& loc,
                               std::string_view msg) {
    return fmt::format("{}:{}: {}: Assertion `{}` failed.{}{}\n",
                       extract_basename(loc.file_name()),
                       loc.line(),
                       loc.function_name(),
                       expr_str,
                       msg.empty() ? "" : "\n",
                       msg);
}