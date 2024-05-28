#pragma once

#include <set>

#include <asio/strand.hpp>
#include <asio/any_io_executor.hpp>
#include <asio/any_completion_handler.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/awaitable.hpp>

#include "request/session.h"
#include "core/core.h"
#include "media_cache/database.h"
#include "media_cache/writer.h"

namespace media_cache
{
class Connection;
class Server : public std::enable_shared_from_this<Server>, NoCopy {
public:
    Server(asio::any_io_executor ex, rc<request::Session>);
    ~Server();

    void start(std::filesystem::path cache_dir, rc<DataBase>);
    void stop();
    auto port() const -> i32;

private:
    void connection_done(rc<Connection>);
    auto listener(rc<DataBase>) -> asio::awaitable<void>;

    asio::any_io_executor               m_ex;
    asio::strand<asio::any_io_executor> m_strand;

    std::set<rc<Connection>> m_connections;
    rc<Writer>               m_writer;

    asio::ip::tcp::acceptor m_acceptor;
    i32                     m_port;
    rc<request::Session>    m_session;
    std::filesystem::path   m_cache_dir;
};

} // namespace media_cache
