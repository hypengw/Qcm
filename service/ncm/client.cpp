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

namespace
{

std::string concat_query(std::string_view url, std::string_view query) {
    return fmt::format("{}{}{}", url, query.empty() ? "" : "?", query);
}

} // namespace
// ""

Client::Client(rc<Session> sess, asio::any_io_executor ex)
    : m_session(sess),
      m_csrf(std::make_shared<std::string>()),
      m_crypto(std::make_shared<Crypto>()),
      m_ex(std::make_shared<executor_type>(ex)) {
    m_req_common.set_connect_timeout(30)
        .set_transfer_timeout(60)
        .set_header("Referer", "https://music.163.com")
        .set_header("User-Agent",
                    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) AppleWebKit/605.1.15 (KHTML, "
                    "like Gecko) Version/13.1.2 Safari/605.1.15");
}

Client::~Client() {}

Client::executor_type& Client::get_executor() { return *m_ex; }

template<>
request::Request Client::make_req<api::CryptoType::WEAPI>(std::string_view          path,
                                                          const request::UrlParams& q) const {
    Request req { m_req_common };
    req.set_url(concat_query(path, q.encode()));
    return req;
}
template<>
request::Request Client::make_req<api::CryptoType::EAPI>(std::string_view          path,
                                                         const request::UrlParams& q) const {
    Request req { m_req_common };
    req.set_url(concat_query(path, q.encode()))
        .set_header("Cookie", "os=pc; appver=2.10.6; versioncode=200601;");
    return req;
}
template<>
request::Request Client::make_req<api::CryptoType::NONE>(std::string_view          path,
                                                         const request::UrlParams& q) const {
    return Request { m_req_common }.set_url(concat_query(path, q.encode()));
}

template<>
std::optional<std::string> Client::encrypt<api::CryptoType::WEAPI>(std::string_view,
                                                                   const Params& p) {
    // p.insert_or_assign(std::string("csrf_token"), *csrf);
    return m_crypto->weapi(To<std::vector<byte>>::from(to_json_str(p)));
}
template<>
std::optional<std::string> Client::encrypt<api::CryptoType::EAPI>(std::string_view path,
                                                                  const Params&    p) {
    assert(path.starts_with("/eapi"));
    path.remove_prefix(5);
    std::string path_ = fmt::format("/api{}", path);
    return m_crypto->eapi(path_, To<std::vector<byte>>::from(to_json_str(p)));
}
template<>
std::optional<std::string> Client::encrypt<api::CryptoType::NONE>(std::string_view, const Params&) {
    return std::nullopt;
}

awaitable<rc<request::Response>> Client::rsp(const request::Request& q) const {
    rc<request::Response> rsp;
    UNWRAP(rsp, co_await m_session->get(q));
    co_return rsp;
}

awaitable<Result<std::vector<byte>>> Client::post(const request::Request& req,
                                                  std::string_view        body) {
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
        co_return nstd::unexpected(Error::push(ec.message()));
    else
        co_return To<std::vector<byte>>::from(buf);
}
