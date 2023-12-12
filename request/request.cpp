#include "request.h"
#include "request_p.h"

#include <iostream>
#include <fmt/core.h>

#include "session.h"
#include "session_p.h"

#include "response.h"
#include "curl_multi.h"
#include "curl_easy.h"

#include "core/type_list.h"
#include "core/variant_helper.h"

using namespace request;

std::error_code request::global_init() {
    return ::make_error_code(curl_global_init(CURL_GLOBAL_ALL));
}

Request::Request() noexcept: m_d(std::make_unique<Private>(this)) {}
Request::Request(std::string_view url) noexcept: Request() { set_url(url); }
Request::~Request() noexcept {}
Request::Request(const Request& o): m_d(std::make_unique<Private>(*(o.m_d))) { m_d->m_q = this; }
Request& Request::operator=(const Request& o) {
    *(m_d)   = *(o.m_d);
    m_d->m_q = this;
    return *this;
}

Request::Private::Private(Request* q)
    : m_q(q),
      m_opts { req_opt::Timeout { .low_speed = 30, .connect_timeout = 180, .transfer_timeout = 0 },
               req_opt::Proxy {},
               req_opt::Tcp { .keepalive = false, .keepidle = 120, .keepintvl = 60 } } {}
Request::Private::~Private() {}

std::string_view Request::url() const {
    C_D(const Request);
    return d->m_uri.uri;
}

const URI& Request::url_info() const {
    C_D(const Request);
    return d->m_uri;
}

Request& Request::set_url(std::string_view uri) {
    C_D(Request);
    d->m_uri = URI::from(uri);
    return *this;
}

std::string Request::header(std::string_view name) const {
    C_D(const Request);
    if (d->m_header.contains(name)) {
        return d->m_header.at(std::string(name));
    }
    return std::string();
}

const Header& Request::header() const {
    C_D(const Request);
    return d->m_header;
}

Request& Request::set_header(std::string_view name, std::string_view value) {
    C_D(Request);
    d->m_header.insert_or_assign(std::string(name), value);
    return *this;
}

void Request::set_option(const Header& header) {
    C_D(Request);
    d->m_header = header;
}

const_voidp Request::get_opt(usize idx) const {
    C_D(const Request);
    return RequestOpts::runtime_select(idx, [d]<usize I, typename T>() -> const_voidp {
        return &std::get<I>(d->m_opts);
    });
}
voidp Request::get_opt(usize idx) {
    C_D(Request);
    return RequestOpts::runtime_select(idx, [d]<usize I, typename T>() -> voidp {
        return &std::get<I>(d->m_opts);
    });
}

void Request::set_opt(const RequestOpt& opt) {
    C_D(Request);

    std::get<req_opt::Proxy>(d->m_opts);
    std::visit(overloaded { [d](const auto& t) {
                   std::get<std::decay_t<decltype(t)>>(d->m_opts) = t;
               } },
               opt);
}