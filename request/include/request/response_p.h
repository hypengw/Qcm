#pragma once

#include <atomic>
#include "request/response.h"
#include "request/session.h"

namespace request
{

class Connection;
class Response::Private {
public:
    Private(Response*, const Request&, Operation, rc<Session>) noexcept;
    C_DECLARE_PUBLIC(Response, m_q)
    void set_share(const std::optional<SessionShare>& share) { m_share = share; }

private:
    Response* m_q;
    Request   m_req;

    Operation m_operation;
    bool      m_finished;

    asio::streambuf             m_send_buffer;
    rc<Connection>              m_connect;
    std::optional<SessionShare> m_share;

    std::pmr::polymorphic_allocator<char> m_allocator;
};

} // namespace request
