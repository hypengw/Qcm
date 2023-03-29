#pragma once

#include <optional>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>

#include "core/core.h"

namespace media_cache
{
struct GetRequest {
public:
    static asio::awaitable<GetRequest> read(asio::ip::tcp::socket&);

    bool partial() const;

    std::string header;
    std::string path;

    std::optional<std::string> proxy_url;
    std::optional<std::string> proxy_id;
    std::optional<i64>         range_start;
};
} // namespace media_cache
