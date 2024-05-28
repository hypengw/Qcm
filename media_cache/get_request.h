#pragma once

#include <optional>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>

#include "core/core.h"
#include "request/http_header.h"

namespace media_cache
{
struct GetRequest {
public:
    static auto read(asio::ip::tcp::socket&) -> asio::awaitable<GetRequest>;

    auto partial() const -> bool;

    std::string         header_str;
    std::string         path;
    request::HttpHeader header;

    std::optional<std::string> proxy_url;
    std::optional<std::string> proxy_id;
    std::optional<i64>         range_start;
};
} // namespace media_cache
