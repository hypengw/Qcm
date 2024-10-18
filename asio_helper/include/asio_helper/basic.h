#pragma once

#include <asio/co_spawn.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/bind_executor.hpp>
#include <asio/strand.hpp>
#include <asio/deferred.hpp>
#include <asio/detached.hpp>
#include <asio/error_code.hpp>

#include "asio_helper/detached_log.h"