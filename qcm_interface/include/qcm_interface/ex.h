#pragma once

#include <asio/thread_pool.hpp>
#include <asio/strand.hpp>
#include "asio_qt/qt_executor.h"
#include "qcm_interface/export.h"

namespace qcm
{

QCM_INTERFACE_API auto qexecutor() -> QtExecutor&;
QCM_INTERFACE_API auto pool_executor() -> asio::thread_pool::executor_type;
QCM_INTERFACE_API auto strand_executor() -> asio::strand<asio::thread_pool::executor_type>;

} // namespace qcm