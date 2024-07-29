#include "media_cache/server.h"

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/as_tuple.hpp>

#include "core/log.h"

#include "connection.h"

using namespace media_cache;

Server::Server(executor_type ex, rc<request::Session> s)
    : m_ex(ex),
      m_strand(ex),
      m_writer(make_rc<Writer>()),
      m_acceptor(m_strand),
      m_port(0),
      m_session(s) {}

Server::~Server() {}

i32 Server::port() const { return m_port; }

void Server::stop() {
    DEBUG_LOG("media_cache server stop");
    m_acceptor.close();
    for (auto& c : m_connections) {
        c->stop();
    }
    m_connections.clear();
}

void Server::connection_done(rc<Connection> cnt) {
    auto self = shared_from_this();
    asio::dispatch(m_strand, [this, self, cnt]() {
        m_connections.erase(cnt);
    });
}

asio::awaitable<void> Server::listener(rc<DataBase> db) {
    auto self = shared_from_this();
    for (;;) {
        auto [ec, socket] = co_await m_acceptor.async_accept(asio::as_tuple(asio::use_awaitable));
        if (ec) {
            if (ec != asio::error::operation_aborted) {
                ERROR_LOG("{}", ec.message());
            }
            break;
        }
        auto c = std::make_shared<Connection>(std::move(socket), db);
        asio::co_spawn(
            asio::make_strand(m_ex),
            [c, this]() -> asio::awaitable<void> {
                co_await c->run(m_session, m_writer, m_cache_dir);
            },
            [self, c](std::exception_ptr p) {
                if (p) {
                    try {
                        std::rethrow_exception(p);
                    } catch (const std::exception& e) {
                        DEBUG_LOG("{}", e.what());
                    }
                }
                if (auto& req = c->get_req(); req) {
                    DEBUG_LOG("done {}", req.value().path);
                }
                self->connection_done(c);
            });
    }
    co_return;
}

void Server::start(std::filesystem::path cache_dir, rc<DataBase> db) {
    m_cache_dir = cache_dir;

    auto addr = asio::ip::make_address_v4(asio::ip::address_v4::bytes_type { 127, 0, 0, 1 });
    auto end  = asio::ip::tcp::endpoint(addr, 0);
    m_acceptor.open(end.protocol());
    m_acceptor.bind(end);
    m_acceptor.listen();

    auto local_end = m_acceptor.local_endpoint();
    m_port         = local_end.port();

    auto self = shared_from_this();
    asio::co_spawn(
        m_strand,
        [self, db]() -> asio::awaitable<void> {
            co_await self->listener(db);
            co_return;
        },
        asio::detached);
}
