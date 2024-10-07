#include "response.h"

#include <fmt/core.h>
#include <cstdio>
#include <asio_helper/helper.h>
#include "core/str_helper.h"

#include "response_p.h"
#include "request_p.h"
#include "session.h"

#include "connection.h"

#include "curl_easy.h"
#include "curl_error.h"

#include <assert.h>

using namespace request;

namespace
{

void apply_easy_request(Response::Private* rsp, CurlEasy& easy, const Request& req) {
    easy.setopt(CURLOPT_URL, req.url().data());
    {
        auto& timeout = req.get_opt<req_opt::Timeout>();

        easy.setopt(CURLOPT_LOW_SPEED_LIMIT, timeout.low_speed);
        easy.setopt(CURLOPT_LOW_SPEED_TIME, timeout.transfer_timeout);
        easy.setopt(CURLOPT_CONNECTTIMEOUT, timeout.connect_timeout);
    }
    {
        auto& tcp = req.get_opt<req_opt::Tcp>();
        easy.setopt(CURLOPT_TCP_KEEPALIVE, tcp.keepalive);
        easy.setopt(CURLOPT_TCP_KEEPIDLE, tcp.keepidle);
        easy.setopt(CURLOPT_TCP_KEEPINTVL, tcp.keepintvl);
    }
    {
        auto& p = req.get_opt<req_opt::Proxy>();
        easy.setopt(CURLOPT_PROXYTYPE, p.type);
        easy.setopt(CURLOPT_PROXY, p.content.empty() ? NULL : p.content.c_str());
    }
    {
        auto& p = req.get_opt<req_opt::SSL>();
        easy.setopt(CURLOPT_SSL_VERIFYPEER, (long)p.verify_certificate);
        easy.setopt(CURLOPT_PROXY_SSL_VERIFYPEER, (long)p.verify_certificate);
    }
    {
        auto& p = req.get_opt<req_opt::Share>();
        if (p.share) {
            easy.setopt<CURLOPT_SHARE>(p.share->handle());
        }
        rsp->set_share(p.share);
    }
    easy.set_header(req.header());
}

template<Attribute A, CURLINFO Info = to_curl_info(A)>
attr_value attr_from_easy(CurlEasy& easy) {
    auto res = easy.template get_info<attr_type<A>>(Info);
    return res
        .map([](auto a) {
            return attr_value(a);
        })
        .value_or(attr_value { std::monostate {} });
}

} // namespace

Response::Private::Private(Response* res, const Request& req, Operation oper,
                           rc<Session> ses) noexcept
    : m_q(res),
      m_req(req),
      m_operation(oper),
      m_connect(
          std::make_shared<Connection>(ses->get_executor(), ses->channel_rc(), ses->allocator())),
      m_allocator(ses->allocator()) {}

Response::Response(const Request& req, Operation oper, rc<Session> ses) noexcept
    : m_d(std::make_unique<Private>(this, req, oper, ses)) {
    C_D(Response);
    auto& easy = connection().easy();
    switch (oper) {
    case Operation::GetOperation: break;
    case Operation::PostOperation:
        easy.setopt(CURLOPT_POST, 1);
        easy.setopt(CURLOPT_POSTFIELDS, NULL);
        easy.setopt(CURLOPT_POSTFIELDSIZE_LARGE, 0);
        break;
    default: break;
    }
    apply_easy_request(d, easy, req);
    {
        auto& p = req.get_opt<req_opt::Read>();
        if (p.callback) {
            connection().set_send_callback(p.callback);
        }
    }
}

Response::~Response() noexcept { cancel(); }

auto Response::allocator() const -> const std::pmr::polymorphic_allocator<char>& {
    C_D(const Response);
    return d->m_allocator;
}

rc<Response> Response::make_response(const Request& req, Operation oper, rc<Session> ses) {
    return std::make_shared<Response>(req, oper, ses);
}

const Request& Response::request() const {
    C_D(const Response);
    return d->m_req;
}

bool Response::pause_send(bool) {
    //    C_D(Response);
    //    return d->m_easy->pause(val ? CURLPAUSE_SEND : CURLPAUSE_SEND_CONT) == CURLE_OK;
    return true;
}
bool Response::pause_recv(bool) {
    //    C_D(Response);
    //    return d->m_easy->pause(val ? CURLPAUSE_RECV : CURLPAUSE_RECV_CONT) == CURLE_OK;
    return true;
}

void Response::add_send_buffer(asio::const_buffer buf) {
    C_D(Response);
    auto& send_buf = d->m_send_buffer;
    send_buf.commit(asio::buffer_copy(send_buf.prepare(buf.size()), buf));
}

void Response::async_read_some_impl(
    asio::mutable_buffer                                        buffer,
    asio::any_completion_handler<void(asio::error_code, usize)> handler) {
    C_D(Response);

    connection().async_read_some(buffer, std::move(handler));
}

void Response::async_write_some_impl(
    asio::const_buffer                                          buffer,
    asio::any_completion_handler<void(asio::error_code, usize)> handler) {
    C_D(Response);
    connection().async_write_some(buffer, std::move(handler));
}

void Response::prepare_perform() {
    C_D(Response);
    auto& easy = connection().easy();

    switch (d->m_operation) {
    case Operation::GetOperation: break;
    case Operation::PostOperation: {
        auto& p = d->m_req.get_opt<req_opt::Read>();
        if (p.callback) {
            easy.setopt(CURLOPT_POSTFIELDSIZE_LARGE, p.size ? p.size : -1);
        } else {
            auto& send_buffer = d->m_send_buffer;
            easy.setopt(CURLOPT_POSTFIELDS, send_buffer.data());
            easy.setopt(CURLOPT_POSTFIELDSIZE_LARGE, send_buffer.size());
        }
        break;
    }
    default: break;
    }

    connection().set_url(d->m_req.url());
}

Operation Response::operation() const {
    C_D(const Response);
    return d->m_operation;
}

Response::executor_type& Response::get_executor() {
    C_D(Response);
    return connection().get_executor();
}

bool Response::is_finished() const {
    C_D(const Response);
    return false;
}

attr_value Response::attribute(Attribute A) const {
    C_D(const Response);
    auto& easy = connection().easy();
    switch (A) {
        using enum Attribute;
    case HttpCode: return attr_from_easy<HttpCode>(easy); break;
    default: break;
    }
    return {};
}

rc<Response> Response::get_rc() { return shared_from_this(); }

auto Response::header() const -> const HttpHeader& { return connection().header(); }
auto Response::code() const -> std::optional<i32> {
    auto& start = this->header().start;
    if (start) {
        if (auto status = std::get_if<HttpHeader::Status>(&start.value())) {
            return status->code;
        }
    }
    return std::nullopt;
}
auto Response::connection() -> Connection& {
    C_D(Response);
    return *(d->m_connect);
}
auto Response::connection() const -> const Connection& {
    C_D(const Response);
    return *(d->m_connect);
}

void Response::cancel() { connection().about_to_cancel(); }
