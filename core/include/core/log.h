#pragma once

#include "core/macro.h"

// clang-format off
#define GENERIC_LOG(lv, ...) if(qcm::log::log_check(lv)) qcm::log::log(lv, rstd::cppstd::source_location::current(), __VA_ARGS__);

#define LOG_ERROR(...)   GENERIC_LOG(qcm::LogLevel::ERROR,   __VA_ARGS__)
#define LOG_WARN(...)    GENERIC_LOG(qcm::LogLevel::WARN, __VA_ARGS__)
#define LOG_INFO(...)    GENERIC_LOG(qcm::LogLevel::INFO,    __VA_ARGS__)
#define LOG_DEBUG(...)   GENERIC_LOG(qcm::LogLevel::DEBUG,   __VA_ARGS__)
// clang-format on

#define QCM_LOG_IMPL                                                \
    namespace qcm                                                   \
    {                                                               \
    struct LogManagerImpl final : LogManager {                      \
        auto level() const -> LogLevel override { return m_level; } \
        void set_level(LogLevel v) override { m_level = v; }        \
                                                                    \
        LogLevel m_level { LogLevel::INFO };                        \
    };                                                              \
    }                                                               \
    auto the_log_manager() -> qcm::LogManager* {                    \
        static qcm::LogManagerImpl manager;                         \
        return &manager;                                            \
    }
