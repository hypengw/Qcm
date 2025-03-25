#pragma once

#include <stdexcept>
#include <source_location>

#include <asio/any_completion_handler.hpp>

#include "core/asio/export.h"

namespace helper
{

QCM_ASIO_API void
handle_asio_exception(std::exception_ptr                                   ptr,
                      asio::any_completion_handler<void(std::string_view)> on_error = {},
                      const std::source_location loc = std::source_location::current());

}