#pragma once
#include <string_view>
#include <source_location>
#include <cstdlib>

#include "core/fmt.h"
#include "core/macro.h"

#ifdef __clangd__
#    include "core/clangd.h"
#endif

import qcm.log;

// clang-format off
#define GENERIC_LOG(lv, ...) if(qcm::log::log_check(lv)) qcm::log::log(lv, std::source_location::current(), __VA_ARGS__);

#define LOG_ERROR(...)   GENERIC_LOG(qcm::LogLevel::ERROR,   __VA_ARGS__)
#define LOG_WARN(...)    GENERIC_LOG(qcm::LogLevel::WARN, __VA_ARGS__)
#define LOG_INFO(...)    GENERIC_LOG(qcm::LogLevel::INFO,    __VA_ARGS__)
#define LOG_DEBUG(...)   GENERIC_LOG(qcm::LogLevel::DEBUG,   __VA_ARGS__)
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
    qcm::log::handle_assert<ENABLED>(             \
        [&]() -> bool {                           \
            return (bool)(EXPR);                  \
        },                                        \
        #EXPR,                                    \
        [&]() -> std::string {                    \
            return std::format(__VA_ARGS__);      \
        },                                        \
        LOC)

#define _assert_(EXPR) _tpl_assert_(qcm::log::enable_debug, EXPR, std::source_location::current())
#define _assert_msg_(EXPR, ...) \
    _tpl_assert_msg_(qcm::log::enable_debug, EXPR, std::source_location::current(), __VA_ARGS__)

#define _assert_rel_(EXPR) _tpl_assert_(true, EXPR, std::source_location::current())
#define _assert_msg_rel_(EXPR, ...) \
    _tpl_assert_msg_(true, EXPR, std::source_location::current(), __VA_ARGS__)

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
