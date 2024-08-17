#pragma once

#include <string>
#include <optional>

#include <asio/awaitable.hpp>

namespace media_cache
{

class DataBase {
public:
    struct Item {
        std::string key;
        std::string content_type;
        std::size_t content_length;
    };
    virtual auto get_executor() -> asio::any_io_executor                      = 0;
    virtual auto get(std::string key) -> asio::awaitable<std::optional<Item>> = 0;
    virtual auto insert(Item) -> asio::awaitable<void>                        = 0;
};
} // namespace media_cache
