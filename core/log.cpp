#include "core/log.h"

#include <cassert>
#include <cstdio>

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

void qcm::generic_log(qcm::LogLevel level, std::string_view file, int line,
                      std::string_view fmted) {
    bool is_stderr = check_stderr(level);
    if (is_stderr) {
        fmt::print(stderr, "{} at {}({}): {}\n", to_sv(level), file, line, fmted);
        std::fflush(stderr);
    } else {
        fmt::print("{} at {}({}): {}\n", to_sv(level), file, line, fmted);
    }
}

bool qcm::handle_assert(std::string_view func, std::string_view file, int line,
                        std::string_view expr, std::string_view reason) {
    fmt::print(stderr, "assert '{}' at {} {}:{}, {}\n", expr, func, file, line, reason);
    std::fflush(stderr);
    return false;
}

void qcm::crash() {
    bool ok { false };
    assert(ok);
}
