#pragma once

#include <asio/thread_pool.hpp>
#include <asio/strand.hpp>
#include "core/asio/task.h"
#include "core/qasio/qt_executor.h"
#include "qcm_interface/export.h"

namespace qcm
{

auto qexecutor_switch() -> task<void>;
auto qexecutor() -> QtExecutor&;
auto pool_executor() -> asio::thread_pool::executor_type;
auto strand_executor() -> asio::strand<asio::thread_pool::executor_type>;

} // namespace qcm