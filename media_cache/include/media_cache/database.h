#pragma once

#include <string>
#include <string_view>
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
    virtual asio::awaitable<std::optional<Item>> get(std::string key) = 0;
    virtual asio::awaitable<void>                insert(Item)         = 0;
};
} // namespace media_cache
