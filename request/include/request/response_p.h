#pragma once
#include "response.h"

namespace request
{

class Response::Private {
public:
    using header_handler_type = asio::any_completion_handler<ret_header>;
    using recv_handler_type =
        asio::any_completion_handler<void(asio::error_code, asio::const_buffer, std::size_t*)>;

    Private(Response*, const Request&, Operation, rc<Session>) noexcept;
    C_DECLARE_PUBLIC(Response, m_q)

private:
    static std::size_t header_callback(char* ptr, std::size_t size, std::size_t nmemb,
                                       Response* self);
    static std::size_t write_callback(char* ptr, std::size_t size, std::size_t nmemb,
                                      Response* self);
    static std::size_t read_callback(char* ptr, std::size_t size, std::size_t nmemb,
                                     Response* self);

    Response*     m_q;
    Request       m_req;
    up<CurlEasy>  m_easy;
    weak<Session> m_session;

    Operation     m_operation;
    bool          m_finished;
    Header        m_header;
    CookieJar     m_cookie_jar;

    asio::streambuf m_send_buffer;
    asio::streambuf m_recv_buffer;

    header_handler_type m_header_handler;
    recv_handler_type   m_recv_handler;
};

} // namespace request
