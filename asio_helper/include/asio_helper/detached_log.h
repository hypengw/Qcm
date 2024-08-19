#pragma once
#include <exception>
#include "asio_helper/export.h"

namespace helper
{
class QCM_ASIO_API asio_detached_log_t {
public:
    void operator()(std::exception_ptr);
};

QCM_ASIO_API extern asio_detached_log_t asio_detached_log;
} // namespace helper