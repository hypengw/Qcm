#pragma once
#include <exception>
#include <source_location>
#include "asio_helper/export.h"

namespace helper
{
class QCM_ASIO_API asio_detached_log_t {
public:
    void operator()(std::exception_ptr, const std::source_location loc = {});
};

QCM_ASIO_API extern asio_detached_log_t asio_detached_log;
} // namespace helper