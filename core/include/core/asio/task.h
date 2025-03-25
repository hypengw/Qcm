#pragma once

#include <asio/use_awaitable.hpp>
#include <asio/as_tuple.hpp>

namespace qcm
{
template<typename T, typename Executor = asio::any_io_executor>
using task = asio::awaitable<T, Executor>;

template<typename CompletionToken>
auto as_tuple(CompletionToken&& t) {
    return asio::as_tuple(std::forward<CompletionToken>(t));
};

constexpr asio::use_awaitable_t<> use_task;
} // namespace qcm