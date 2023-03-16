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
    Url      m_url;
    Header   m_header;
    i32      m_low_speed; // byte
    i32      m_connect_timeout;
    i32      m_transfer_timeout;
};
} // namespace request
