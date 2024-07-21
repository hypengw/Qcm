#pragma once

#include <asio/streambuf.hpp>
#include <asio/buffers_iterator.hpp>
#include <asio/bind_executor.hpp>
#include <asio/awaitable.hpp>
#include <asio/post.hpp>

#include <string>
#include <concepts>

#include "core/core.h"
#include "core/expected_helper.h"
#include "core/str_helper.h"

DEFINE_CONVERT(std::vector<byte>, asio::streambuf) {
    out.clear();
    std::transform(asio::buffers_begin(in.data()),
                   asio::buffers_end(in.data()),
                   std::back_inserter(out),
                   [](unsigned char c) {
                       return byte { c };
                   });
}

template<>
struct fmt::formatter<asio::streambuf> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const asio::streambuf& buf, FormatContext& ctx) const {
        std::string out { asio::buffers_begin(buf.data()), asio::buffers_end(buf.data()) };
        return fmt::formatter<std::string>::format(out, ctx);
    }
};

namespace helper
{
template<typename T>
concept is_awaitable = ycore::is_specialization_of<T, asio::awaitable>;

template<typename T>
concept is_sync_stream = requires(T s, asio::const_buffer buf) {
    { s.write_some(buf) } -> std::convertible_to<std::size_t>;
};

template<typename Ex, typename ExWork, typename F, typename... Args>
void post_via(const Ex& exec, const ExWork& work_exec, F&& handler, Args&&... args) {
    asio::post(exec,
               asio::bind_executor(
                   work_exec, std::bind(std::forward<F>(handler), std::forward<Args>(args)...)));
}

template<typename Ex, typename F, typename... Args>
void post(const Ex& exec, F&& handler, Args&&... args) {
    post_via(exec,
             asio::get_associated_executor(handler, exec),
             std::forward<F>(handler),
             std::forward<Args>(args)...);
}

namespace detail_awaitable
{

template<typename T>
struct inner_type {
    using type = T;
};

template<typename T>
using inner_type_t = typename inner_type<T>::type;

template<typename T, typename F>
struct inner_type<nstd::expected<T, F>> {
    using type = inner_type_t<T>;
};

template<typename T>
struct inner_type<asio::awaitable<T>> {
    using type = inner_type_t<T>;
};
} // namespace detail_awaitable

template<typename T, typename F, typename R = detail_awaitable::inner_type_t<T>>
asio::awaitable<nstd::expected<R, F>> awaitable_unpack(nstd::expected<T, F>&& expr) {
    if (expr.has_value()) {
        if constexpr (helper::is_awaitable<T>)
            co_return co_await awaitable_unpack(
                nstd::expected_unpack(co_await std::move(expr).value()));
        else
            co_return nstd::expected_unpack(std::move(expr));
    } else {
        co_return nstd::unexpected(std::move(expr).error());
    }
}
} // namespace helper
