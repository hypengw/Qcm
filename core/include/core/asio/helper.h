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
#include "core/helper.h"

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
struct std::formatter<asio::streambuf> : std::formatter<std::string> {
    auto format(const asio::streambuf& buf, format_context& ctx) const -> format_context::iterator {
        std::string out { asio::buffers_begin(buf.data()), asio::buffers_end(buf.data()) };
        return std::formatter<std::string>::format(out, ctx);
    }
};

namespace helper
{
template<typename T>
concept is_awaitable = ycore::is_specialization_of_v<T, asio::awaitable>;

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
} // namespace helper