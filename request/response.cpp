#include "response.h"

#include <fmt/core.h>
#include <cstdio>
#include <asio_helper/helper.h>
#include "core/str_helper.h"

#include "response_p.h"
#include "request_p.h"
#include "session.h"

#include "curl_easy.h"
#include "curl_error.h"

#include <assert.h>

using namespace request;

namespace
{

void apply_easy_request(CurlEasy& easy, const Request& req) {
    easy.setopt(CURLOPT_URL, req.url().data());
    easy.setopt(CURLOPT_LOW_SPEED_LIMIT, req.transfer_low_speed());
    easy.setopt(CURLOPT_LOW_SPEED_TIME, req.transfer_timeout());
    easy.setopt(CURLOPT_CONNECTTIMEOUT, req.connect_timeout());
    easy.set_header(req.header());
}

template<Attribute A, CURLINFO Info = to_curl_info(A)>
attr_value attr_from_easy(CurlEasy& easy) {
    auto res = easy.get_info<attr_type<A>>(Info);
    return res
        .map([](auto a) {
            return attr_value(a);
        })
        .value_or(attr_value {});
}

} // namespace

Response::Private::Private(Response* res, const Request& req, Operation oper,
                           rc<Session> ses) noexcept
    : m_q(res),
      m_req(req),
      m_easy(std::make_unique<CurlEasy>(ses->get_executor())),
      m_session(ses),
      m_operation(oper),
      m_finished(false) {}

Response::Response(const Request& req, Operation oper, rc<Session> ses) noexcept
    : m_d(std::make_unique<Private>(this, req, oper, ses)) {
    C_D(Response);
    auto& easy = *(d->m_easy);
    easy.setopt(CURLOPT_WRITEFUNCTION, Response::Private::write_callback);
    easy.setopt(CURLOPT_WRITEDATA, this);

    easy.setopt(CURLOPT_HEADERFUNCTION, Response::Private::header_callback);
    easy.setopt(CURLOPT_HEADERDATA, this);

    // easy.setopt(CURLOPT_READFUNCTION, Response::Private::read_callback);
    // easy.setopt(CURLOPT_READDATA, this);

    switch (oper) {
    case Operation::GetOperation: break;
    case Operation::PostOperation:
        easy.setopt(CURLOPT_POST, 1);
        easy.setopt(CURLOPT_POSTFIELDSIZE, 0);
        break;
    }

    easy.set_done_callback([this](CURLcode cc) {
        this->done(cc);
    });

    apply_easy_request(easy, req);
}

Response::~Response() noexcept {}

rc<Response> Response::make_response(const Request& req, Operation oper, rc<Session> ses) {
    return std::make_shared<Response>(req, oper, ses);
}

const Request& Response::request() const {
    C_D(const Response);
    return d->m_req;
}

std::size_t Response::Private::write_callback(char* ptr, std::size_t size, std::size_t nmemb,
                                              Response* self) {
    C_DP(Response, self);

    asio::const_buffer buffer { ptr, size * nmemb };
    if (buffer.size() == 0) return 0;

    const std::size_t ori_size = buffer.size();

    auto& recv_buffer  = d->m_recv_buffer;
    auto& recv_handler = d->m_recv_handler;

    std::size_t cached = 0;

    if (recv_handler) {
        std::size_t copied {};
        recv_handler(asio::error_code {}, buffer, &copied);
        buffer += copied;
    }

    if (buffer.size() > 0) {
        cached = asio::buffer_copy(recv_buffer.prepare(buffer.size()), buffer);
        buffer += cached;
    }

    if (buffer.size() > 0) {
        DEBUG_LOG("write_callback pause");
        return CURL_WRITEFUNC_PAUSE;
    }

    if (cached > 0) {
        recv_buffer.commit(cached);
    }
    return ori_size;
}

std::size_t Response::Private::header_callback(char* ptr, std::size_t size, std::size_t nmemb,
                                               Response* self) {
    C_DP(Response, self);
    std::string_view header { ptr, size * nmemb };
    if (! header.empty()) {
        if (auto pos = header.find_first_of(':'); pos != std::string_view::npos) {
            auto iter  = header.begin() + pos;
            auto name  = helper::trims(std::string_view { header.begin(), iter });
            auto value = helper::trims(std::string_view { iter + 1, header.end() });

            if (helper::starts_with_i(name, "set-cookie")) {
                d->m_cookie_jar.raw_cookie.append(header).push_back('\n');
            } else {
                d->m_header.insert({ std::string { name }, std::string { value } });
            }
        }
    }
    if (header == "\r\n" || header == "\n") {
        if (d->m_header_handler) {
            d->m_header_handler(asio::error_code {}, d->m_header);
        }
    }
    return header.size();
}

std::size_t Response::Private::read_callback(char* ptr, std::size_t size, std::size_t nmemb,
                                             Response* self) {
    C_DP(Response, self);
    std::size_t total  = size * nmemb;
    auto        copied = asio::buffer_copy(asio::buffer(ptr, total), d->m_send_buffer.data());
    d->m_send_buffer.consume(copied);
    return copied;
}

bool Response::pause_send(bool val) {
    C_D(Response);
    return d->m_easy->pause(val ? CURLPAUSE_SEND : CURLPAUSE_SEND_CONT) == CURLE_OK;
}
bool Response::pause_recv(bool val) {
    C_D(Response);
    return d->m_easy->pause(val ? CURLPAUSE_RECV : CURLPAUSE_RECV_CONT) == CURLE_OK;
}

void Response::add_send_buffer(asio::const_buffer buf) {
    C_D(Response);
    auto& send_buf = d->m_send_buffer;
    send_buf.commit(asio::buffer_copy(send_buf.prepare(buf.size()), buf));
}

void Response::async_read_some_impl(
    asio::mutable_buffer                                              buffer,
    asio::any_completion_handler<void(asio::error_code, std::size_t)> handler) {
    C_D(Response);

    auto ex = get_executor();

    auto recv_handler = [ex, buffer, handler = std::move(handler)](asio::error_code   ec,
                                                                   asio::const_buffer recv,
                                                                   std::size_t* p_copied) mutable {
        auto copied = asio::buffer_copy(buffer, recv);
        helper::post(ex, std::move(handler), ec, copied);
        if (p_copied) *p_copied = copied;
    };

    auto& recv_buffer = d->m_recv_buffer;
    auto  input_size  = recv_buffer.size();

    if (input_size > 0) {
        std::size_t copied;
        recv_handler(asio::error_code {}, recv_buffer.data(), &copied);
        recv_buffer.consume(copied);
    } else if (is_finished() || d->m_recv_handler) {
        _assert_(is_finished());
        recv_handler(asio::stream_errc::eof, {}, nullptr);
    } else if (! d->m_recv_handler) {
        d->m_recv_handler = std::move(recv_handler);
        pause_recv(false);
    }
}

void Response::async_get_header_impl(asio::any_completion_handler<ret_header> handler) {
    C_D(Response);
    if (is_finished()) {
        handler(asio::error_code {}, d->m_header);
    } else {
        d->m_header_handler = std::move(handler);
    }
}

void Response::prepare_perform() {
    C_D(Response);
    auto& easy = *(d->m_easy);

    switch (d->m_operation) {
    case Operation::GetOperation: break;
    case Operation::PostOperation:
        auto& send_buffer = d->m_send_buffer;
        easy.setopt(CURLOPT_POSTFIELDS, send_buffer.data());
        easy.setopt(CURLOPT_POSTFIELDSIZE, send_buffer.size());
        break;
    }
}

Operation Response::operation() const {
    C_D(const Response);
    return d->m_operation;
}

CurlEasy& Response::easy() {
    C_D(Response);
    return *(d->m_easy);
}

Response::executor_type& Response::get_executor() {
    C_D(Response);
    return d->m_easy->get_strand();
}

bool Response::is_finished() const {
    C_D(const Response);
    return d->m_finished;
}

attr_value Response::attribute(Attribute A) const {
    C_D(const Response);
    auto& easy = *(d->m_easy);
    switch (A) {
        using enum Attribute;
    case HttpCode: return attr_from_easy<HttpCode>(easy); break;
    default: break;
    }
    return {};
}

const CookieJar& Response::cookie_jar() const {
    C_D(const Response);
    return d->m_cookie_jar;
}

rc<Response> Response::get_rc() { return shared_from_this(); }

void Response::done(int rc_) {
    C_D(Response);
    auto rc = (CURLcode)rc_;

    auto ec = rc == CURLE_OK ? asio::stream_errc::eof : ::make_error_code(rc);

    d->m_finished = true;
    if (d->m_header_handler) {
        d->m_header_handler(ec, d->m_header);
    }

    if (d->m_recv_handler) {
        d->m_recv_handler(ec, asio::const_buffer {}, nullptr);
    }

    if (auto ses = d->m_session.lock()) {
        ses->done(get_rc());
    }
}

void Response::cancel() {
    rc<Response> self = shared_from_this();
    asio::dispatch(get_executor(), [self]() {
        self->done(CURLcode::CURLE_ABORTED_BY_CALLBACK);
    });
}
