#include "ncm/client.h"

#include <regex>

#include "request/request.h"
#include "request/response.h"
#include "core/log.h"
#include "core/str_helper.h"
#include "core/vec_helper.h"
#include "asio_helper/helper.h"
#include "json_helper/helper.h"

#include "ncm/crypto.h"
#include "ncm/model.h"

#include "dump.h"

using namespace ncm;
using namespace request;

Client::Client(rc<Session> sess, asio::any_io_executor ex)
    : m_session(sess),
      m_csrf(make_rc<std::string>()),
      m_crypto(make_rc<Crypto>()),
      m_ex(make_rc<executor_type>(ex)),
      m_req_common(make_rc<Request>()) {
    m_req_common->get_opt<request::req_opt::Timeout>().set_connect_timeout(30).set_transfer_timeout(
        60);
    m_req_common->set_header("Referer", "https://music.163.com")
        .set_header("User-Agent",
                    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) AppleWebKit/605.1.15 (KHTML, "
                    "like Gecko) Version/13.1.2 Safari/605.1.15");
}

Client::~Client() {}

Client::executor_type& Client::get_executor() { return *m_ex; }

template<>
auto Client::make_req<api::CryptoType::WEAPI>(std::string_view          path,
                                              const request::UrlParams& q) const
    -> request::Request {
    Request req { *m_req_common };
    req.set_url(api::concat_query(path, q.encode()));
    return req;
}
template<>
auto Client::make_req<api::CryptoType::EAPI>(std::string_view          path,
                                             const request::UrlParams& q) const
    -> request::Request {
    Request req { *m_req_common };
    req.set_url(api::concat_query(path, q.encode()))
        .set_header("Cookie", "os=pc; appver=2.10.13; versioncode=202675");
    return req;
}
template<>
auto Client::make_req<api::CryptoType::NONE>(std::string_view          path,
                                             const request::UrlParams& q) const
    -> request::Request {
    return Request { *m_req_common }.set_url(api::concat_query(path, q.encode()));
}

template<>
auto Client::encrypt<api::CryptoType::WEAPI>(std::string_view, const Params& p)
    -> std::optional<std::string> {
    // p.insert_or_assign(std::string("csrf_token"), *csrf);
    return m_crypto->weapi(convert_from<std::vector<byte>>(to_json_str(p)));
}

template<>
auto Client::encrypt<api::CryptoType::EAPI>(std::string_view path, const Params& p)
    -> std::optional<std::string> {
    assert(path.starts_with("/eapi"));
    path.remove_prefix(5);
    std::string path_ = fmt::format("/api{}", path);
    return m_crypto->eapi(path_, convert_from<std::vector<byte>>(to_json_str(p)));
}

template<>
auto Client::encrypt<api::CryptoType::NONE>(std::string_view, const Params&)
    -> std::optional<std::string> {
    return std::nullopt;
}

auto Client::rsp(const request::Request& q) const -> awaitable<rc<request::Response>> {
    rc<request::Response> rsp = UNWRAP(co_await m_session->get(q));
    co_return rsp;
}

auto Client::post(const request::Request& req, std::string_view body)
    -> awaitable<Result<std::vector<byte>>> {
    rc<std::string> csrf = m_csrf;

    rc<Response> rsp;
    EC_RET_CO(rsp, co_await m_session->post(req, asio::buffer(body)));

    _assert_(rsp);

    asio::streambuf buf;
    auto [ec, size] = co_await asio::async_read(
        *rsp, buf, asio::transfer_all(), asio::as_tuple(asio::use_awaitable));

    asio::cancellation_state cs = co_await asio::this_coro::cancellation_state;
    if (cs.cancelled() != asio::cancellation_type::none) {
        asio::detail::throw_error(asio::error::operation_aborted);
    }

    if (ec != asio::error::eof && ec)
        co_return nstd::unexpected(Error::push(ec));
    else
        co_return convert_from<std::vector<byte>>(buf);
}
