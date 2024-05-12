#include "get_request.h"

#include <charconv>

#include <asio/read_until.hpp>
#include <asio/as_tuple.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/streambuf.hpp>

#include <ctre.hpp>

#include "core/log.h"
#include "request/type.h"

using namespace media_cache;

namespace
{
static constexpr auto RangeHeaderPattern = ctll::fixed_string { "[R,r]ange:[ ]?bytes=(\\d*)-" };
static constexpr auto PathPattern        = ctll::fixed_string { "GET /(.*) HTTP" };

static constexpr auto UrlPattern = ctll::fixed_string { "([0-9a-zA-Z]+)[?]url=(.+)" };
} // namespace

auto GetRequest::partial() const -> bool { return (bool)range_start; }

auto GetRequest::read(asio::ip::tcp::socket& s) -> asio::awaitable<GetRequest> {
    GetRequest      req;
    asio::streambuf buf;

    for (;;) {
        auto [ec, size] =
            co_await asio::async_read_until(s, buf, '\n', asio::as_tuple(asio::use_awaitable));
        auto line =
            std::string { asio::buffers_begin(buf.data()), asio::buffers_begin(buf.data()) + size };
        buf.consume(size);
        req.header.append(line);
        if (line == "\r\n" || line == "\n") {
            break;
        }

        if (ec) {
            if (ec != asio::error::eof) {
                ERROR_LOG("{}", ec.message());
            }
            break;
        }
    }

    if (auto [whole, path] = ctre::search<PathPattern>(req.header); whole) {
        req.path = path;
        if (auto [whole, id, url] = ctre::match<UrlPattern>(path); whole) {
            req.proxy_url = request::url_decode(url);
            req.proxy_id  = id;
        }
    }
    if (auto [whole, range] = ctre::search<RangeHeaderPattern>(req.header); whole) {
        req.range_start = range.to_number();
    }
    DEBUG_LOG("MediaCache GetRequest:\n{}", req.header);
    co_return req;
}
