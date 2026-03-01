#pragma once

#include <asio/co_spawn.hpp>
#include <asio/bind_executor.hpp>
#include <asio/strand.hpp>
#include <asio/deferred.hpp>
#include <asio/detached.hpp>
#include <asio/error_code.hpp>
#include <asio/any_completion_handler.hpp>
#include <asio/thread_pool.hpp>

#include "core/asio/detached_log.h"
#include "core/asio/task.h"
