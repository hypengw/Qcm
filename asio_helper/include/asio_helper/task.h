#pragma once

#include <asio/use_awaitable.hpp>

namespace qcm
{
template<typename T, typename Executor = asio::any_io_executor>
using task = asio::awaitable<T, Executor>;

constexpr asio::use_awaitable_t<> use_task;
} // namespace qcm