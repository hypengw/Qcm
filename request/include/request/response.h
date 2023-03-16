#pragma once

#include <string>
#include <functional>
#include <iostream>

#include <asio/any_io_executor.hpp>
#include <asio/any_completion_handler.hpp>
#include <asio/dispatch.hpp>
#include <asio/read.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/as_tuple.hpp>
#include <asio/strand.hpp>

#include "asio_helper/helper.h"

#include "core/core.h"
#include "core/callable.h"
#include "core/variant_helper.h"
#include "core/expected_helper.h"

#include "request.h"

namespace request
{

class CurlEasy;
class Session;

class Response : public std::enable_shared_from_this<Response>, NoCopy {
    friend class Session;

public:
    using executor_type = asio::strand<asio::any_io_executor>;
    class Private;

public:
    executor_type& get_executor();

    template<Attribute A, typename T = attr_type<A>>
    std::optional<T> attribute(void) const {
        return helper::to_optional<T>(attribute(A));
    }

    attr_value attribute(Attribute) const;

    using ret_header = void(tl::expected<Header, asio::error_code>);
    template<typename CompletionToken>
    auto async_get_header(CompletionToken&& token) {
        return asio::async_initiate<CompletionToken, ret_header>(
            [this](asio::any_completion_handler<ret_header> handler) {
                async_get_header_impl(std::move(handler));
            },
            token);
    }

    template<typename MB, typename CompletionToken>
        requires asio::is_const_buffer_sequence<MB>::value
    auto async_read_some(const MB& buffer, CompletionToken&& token) {
        using ret = void(asio::error_code, std::size_t);
        return asio::async_initiate<CompletionToken, ret>(
            [&](auto&& handler) {
                asio::mutable_buffer mu_buf = asio::buffer(buffer);
                asio::dispatch(get_executor(),
                               [this, mu_buf, handler = std::move(handler)]() mutable {
                                   async_read_some_impl(mu_buf, std::move(handler));
                               });
            },
            token);
    }

    template<typename SyncWriteStream>
        requires helper::is_sync_stream<SyncWriteStream>
    asio::awaitable<std::size_t> read_to_stream(SyncWriteStream& writer) {
        asio::streambuf buf;

        auto [ec, size] = co_await asio::async_read(
            *this,
            buf,
            [&buf, &writer](const auto& err, std::size_t) -> std::size_t {
                buf.consume(writer.write_some(buf.data()));
                return ! ! err ? 0 : asio::detail::default_max_transfer_size;
            },
            asio::as_tuple(asio::use_awaitable));
        if (ec != asio::stream_errc::eof) {
            cancel();
            asio::detail::throw_error(ec);
        }
        co_return size;
    }

    static rc<Response> make_response(const Request&, Operation, rc<Session>);
    Response(const Request&, Operation, rc<Session>) noexcept;
    ~Response() noexcept;

    bool           is_finished() const;
    const Request& request() const;
    Operation      operation() const;

    const CookieJar& cookie_jar() const;

    bool pause_send(bool);
    bool pause_recv(bool);

    rc<Response> get_rc();

    void cancel();

private:
    CurlEasy& easy();

    void prepare_perform();
    void add_send_buffer(asio::const_buffer);
    void async_read_some_impl(asio::mutable_buffer,
                              asio::any_completion_handler<void(asio::error_code, std::size_t)>);

    void async_get_header_impl(asio::any_completion_handler<ret_header>);

    void done(int);

    void set_cancel_slot();

private:
    C_DECLARE_PRIVATE(Response, m_d)

    up<Private> m_d;
};

} // namespace request
