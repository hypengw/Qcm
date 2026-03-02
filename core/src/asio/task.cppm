export module qcm.asio:task;
export import qcm.core;
export import asio;

export namespace qcm
{
template<typename T, typename Executor = asio::any_io_executor>
using task = asio::awaitable<T, Executor>;

using asio::as_tuple;

constexpr asio::use_awaitable_t<> use_task;
constexpr asio::deferred_t deferred;
} // namespace qcm

