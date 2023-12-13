#pragma once

#include <set>

#include "core/core.h"
#include "request/session.h"

namespace request
{

class Connection;
class CurlMulti;

class Session::Private {
    friend class Session;

public:
    using channel_poll_type =
        asio::experimental::concurrent_channel<executor_type,
                                               void(asio::error_code, SessionMessage)>;

    Private(Session&, executor_type& ex) noexcept;

    asio::awaitable<void> run();
    void                  handle_message(const SessionMessage&);

    void add_connect(const rc<Connection>&);
    void remove_connect(const rc<Connection>&);

private:
    Session&                    m_p;
    up<CurlMulti>               m_curl_multi;
    executor_type               m_ex;
    asio::strand<executor_type> m_strand;
    std::set<rc<Connection>>    m_connect_set;

    asio::thread_pool     m_poll_thread;
    rc<channel_poll_type> m_channel;
    rc<channel_type>      m_channel_with_notify;
    bool                  m_stopped;

    std::optional<req_opt::Proxy> m_proxy;
    bool                          m_ignore_certificate;
};

} // namespace request
