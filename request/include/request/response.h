#pragma once

#include <string>
#include <functional>
#include <iostream>

#include <asio/any_completion_handler.hpp>
#include <asio/dispatch.hpp>
#include <asio/read.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/as_tuple.hpp>
#include <asio/strand.hpp>
#include <asio/thread_pool.hpp>
#include <asio/associated_executor.hpp>

#include "asio_helper/helper.h"

#include "core/core.h"
#include "core/callable.h"
#include "core/variant_helper.h"
#include "core/expected_helper.h"

#include "request/request.h"
#include "request/http_header.h"

namespace request
{

class Session;
class Connection;
class Response : public std::enable_shared_from_this<Response>, NoCopy {
    friend class Session;

public:
    using executor_type  = asio::strand<asio::thread_pool::executor_type>;
    using allocator_type = std::pmr::polymorphic_allocator<char>;
    class Private;
    static constexpr usize ReadSize { 1024 * 16 };

public:
    executor_type& get_executor();

    template<Attribute A, typename T = attr_type<A>>
    auto attribute(void) const -> std::optional<T> {
        return helper::to_optional<T>(attribute(A));
    }

    auto attribute(Attribute) const -> attr_value;

    auto header() const -> const HttpHeader&;
    auto code() const -> std::optional<i32>;

    template<typename MB, typename CompletionToken>
        requires asio::is_const_buffer_sequence<MB>::value
    auto async_read_some(const MB& buffer, CompletionToken&& token) {
        using ret = void(asio::error_code, std::size_t);
        return asio::async_initiate<CompletionToken, ret>(
            [&](auto&& handler) {
                asio::mutable_buffer mu_buf { asio::buffer(buffer) };
                async_read_some_impl(mu_buf, std::move(handler));
            },
            token);
    }

    template<typename MB, typename CompletionToken>
        requires asio::is_const_buffer_sequence<MB>::value
    auto async_write_some(const MB& buffer, CompletionToken&& token) {
        using ret = void(asio::error_code, std::size_t);
        return asio::async_initiate<CompletionToken, ret>(
            [&](auto&& handler) {
                auto const_buf = asio::const_buffer(buffer);
                async_read_some_impl(const_buf, std::move(handler));
            },
            token);
    }

    template<typename SyncWriteStream>
        requires helper::is_sync_stream<SyncWriteStream>
    auto read_to_stream(SyncWriteStream& writer) -> asio::awaitable<usize> {
        asio::basic_streambuf<allocator_type> buf(std::numeric_limits<usize>::max(), allocator());
        buf.prepare(ReadSize);

        auto [ec, size] = co_await asio::async_read(
            *this,
            buf,
            [&buf, &writer](const auto& err, std::size_t) -> std::size_t {
                buf.consume(writer.write_some(buf.data()));
                return ! ! err ? 0 : asio::detail::default_max_transfer_size;
            },
            asio::as_tuple(asio::bind_executor(get_executor(), asio::use_awaitable)));
        if (ec != asio::stream_errc::eof) {
            asio::detail::throw_error(ec);
        }
        co_return size;
    }

    static auto make_response(const Request&, Operation, rc<Session>) -> rc<Response>;
    Response(const Request&, Operation, rc<Session>) noexcept;
    ~Response() noexcept;

    auto is_finished() const -> bool;
    auto request() const -> const Request&;
    auto operation() const -> Operation;

    auto cookie_jar() const -> const CookieJar&;

    auto pause_send(bool) -> bool;
    auto pause_recv(bool) -> bool;

    auto get_rc() -> rc<Response>;

    void cancel();
    auto allocator() const -> const allocator_type&;

private:
    void prepare_perform();
    void add_send_buffer(asio::const_buffer);
    void async_read_some_impl(asio::mutable_buffer,
                              asio::any_completion_handler<void(asio::error_code, usize)>);

    void async_write_some_impl(asio::const_buffer,
                               asio::any_completion_handler<void(asio::error_code, usize)>);

    void done(int);
    auto connection() -> Connection&;
    auto connection() const -> const Connection&;

private:
    C_DECLARE_PRIVATE(Response, m_d)
};

} // namespace request
