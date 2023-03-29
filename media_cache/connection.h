#pragma once

#include <asio/ip/tcp.hpp>
#include <asio/awaitable.hpp>

#include "request/session.h"
#include "core/core.h"

#include "get_request.h"
#include "media_cache/database.h"

namespace media_cache
{
class Server;
class Connection {
public:
    Connection(asio::ip::tcp::socket, rc<DataBase>);
    ~Connection();

    asio::awaitable<void> run(rc<request::Session>, std::filesystem::path cache_dir);

    void                             stop();
    const std::optional<GetRequest>& get_req() { return m_req; }

private:
    asio::awaitable<void> http_source(std::filesystem::path, rc<request::Session>);
    asio::awaitable<void> file_source(std::filesystem::path);

    asio::awaitable<bool> check_cache(std::string_view key, std::filesystem::path);

    asio::ip::tcp::socket     m_s;
    rc<DataBase>              m_db;
    std::optional<GetRequest> m_req;
};

} // namespace media_cache
