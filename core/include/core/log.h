#pragma once

#include <source_location>

#include "core/macro.h"

// clang-format off
#define GENERIC_LOG(lv, ...) if(qcm::log::log_check(lv)) qcm::log::log(lv, std::source_location::current(), __VA_ARGS__);

#define LOG_ERROR(...)   GENERIC_LOG(qcm::LogLevel::ERROR,   __VA_ARGS__)
#define LOG_WARN(...)    GENERIC_LOG(qcm::LogLevel::WARN, __VA_ARGS__)
#define LOG_INFO(...)    GENERIC_LOG(qcm::LogLevel::INFO,    __VA_ARGS__)
#define LOG_DEBUG(...)   GENERIC_LOG(qcm::LogLevel::DEBUG,   __VA_ARGS__)
// clang-format on
