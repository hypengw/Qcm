#pragma once

#include "request.h"

namespace request
{

class Request::Private {
public:
    C_DECLARE_PUBLIC(Request, m_q)
    Private(Request* q);
    ~Private();

private:
    Request* m_q;
    URI      m_uri;
    Header   m_header;
    i64      m_low_speed; // byte
    i64      m_connect_timeout;
    i64      m_transfer_timeout;
    bool     m_tcp_keepalive;
    i64      m_tcp_keepidle;
    i64      m_tcp_keepintvl;
};
} // namespace request
