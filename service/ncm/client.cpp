#include "ncm/client.h"

#include <ranges>

#include "request/request.h"
#include "request/response.h"
#include "request/session_share.h"
#include "core/log.h"
#include "core/random.h"

#include "ncm/crypto.h"
#include "ncm/model.h"

#include "dump.h"

using namespace ncm;
using namespace request;

class Client::Private {
public:
    Private(rc<Session> sess, executor_type ex, std::string device_id)
        : session(sess), device_id(device_id), csrf(), crypto(), ex(ex), req_common() {}
    rc<request::Session> session;
    SessionShare         session_share;
    std::string          device_id;
    std::string          csrf;
    Crypto               crypto;
    executor_type        ex;
    request::Request     req_common;

    std::map<std::string, std::any, std::less<>> props;

    std::map<std::string, std::string, std::less<>> web_params;
    std::map<std::string, std::string, std::less<>> device_params;
};

Client::Client(rc<Session> sess, executor_type ex, std::string device_id)
    : d_ptr(make_rc<Private>(sess, ex, device_id)) {
    C_D(Client);
    d->req_common.get_opt<request::req_opt::Timeout>().set_connect_timeout(30).set_transfer_timeout(
        60);
    d->req_common.set_header("Referer", "https://music.163.com")
        .set_header("User-Agent",
                    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) AppleWebKit/605.1.15 (KHTML, "
                    "like Gecko) Version/13.1.2 Safari/605.1.15");
    d->req_common.set_opt(req_opt::Share { d->session_share });

    auto player_id = std::ranges::transform_view(std::ranges::views::iota(0, 8), [](auto) {
        return qcm::Random::get('0', '9');
    });

    d->web_params    = { { "playerid", std::string(player_id.begin(), player_id.end()) },
                         { "sDeviceId", d->device_id } };
    d->device_params = {
        { "deviceId", d->device_id },
        { "resolution", "1920x1080" },
        { "appver", "2.10.13" },
        { "os", "pc" },
        { "versioncode", "202675" },
        { "osver", "8.1.0" },
        { "brand", "hw" },
        { "model", "" },
        { "channel", "" },
    };
}

Client::~Client() {}

Client::executor_type& Client::get_executor() {
    C_D(Client);
    return (d->ex);
}

template<>
auto Client::make_req<api::CryptoType::WEAPI>(
    std::string_view path, const request::UrlParams& q) const -> request::Request {
    C_D(const Client);
    Request req { d->req_common };

    auto cookie_item = std::ranges::transform_view(d->web_params, [](auto& el) -> std::string {
        return fmt::format("{}={}", el.first, el.second);
    });
    req.set_url(api::concat_query(path, q.encode()))
        .set_header("Cookie", fmt::format("{}", fmt::join(cookie_item, "; ")));

    return req;
}
template<>
auto Client::make_req<api::CryptoType::EAPI>(
    std::string_view path, const request::UrlParams& q) const -> request::Request {
    C_D(const Client);
    Request req { d->req_common };

    auto cookie_item = std::ranges::transform_view(d->device_params, [](auto& el) -> std::string {
        return fmt::format("{}={}", el.first, el.second);
    });

    req.set_url(api::concat_query(path, q.encode()))
        .set_header("Cookie", fmt::format("{}", fmt::join(cookie_item, "; ")));
    return req;
}
template<>
auto Client::make_req<api::CryptoType::NONE>(
    std::string_view path, const request::UrlParams& q) const -> request::Request {
    C_D(const Client);
    return Request { d->req_common }.set_url(api::concat_query(path, q.encode()));
}

template<>
auto Client::encrypt<api::CryptoType::WEAPI>(std::string_view,
                                             const Params& p) -> std::optional<std::string> {
    C_D(Client);
    // p.insert_or_assign(std::string("csrf_token"), *csrf);
    return d->crypto.weapi(convert_from<std::vector<byte>>(to_json_str(p)));
}

template<>
auto Client::encrypt<api::CryptoType::EAPI>(std::string_view path,
                                            const Params&    p) -> std::optional<std::string> {
    C_D(Client);
    std::string path_ = fmt::format("/api{}", path);
    return d->crypto.eapi(path_, convert_from<std::vector<byte>>(to_json_str(p)));
}

template<>
auto Client::encrypt<api::CryptoType::NONE>(std::string_view,
                                            const Params&) -> std::optional<std::string> {
    return std::string {};
}

template<api::CryptoType CT>
auto Client::format_url(std::string_view base, std::string_view path) const -> std::string {
    C_D(const Client);
    std::string_view prefix, suffix;
    if constexpr (CT == api::CryptoType::EAPI) {
        prefix = "/eapi";
    } else if constexpr (CT == api::CryptoType::WEAPI) {
        prefix = "/weapi";
    }
    return std::format("{}{}{}{}", base, prefix, path, suffix);
}

template auto
              Client::format_url<api::CryptoType::WEAPI>(std::string_view base,
                                           std::string_view path) const -> std::string;
template auto Client::format_url<api::CryptoType::EAPI>(std::string_view base,
                                                        std::string_view path) const -> std::string;
template auto Client::format_url<api::CryptoType::NONE>(std::string_view base,
                                                        std::string_view path) const -> std::string;

auto Client::rsp(const request::Request& q) const -> awaitable<rc<request::Response>> {
    C_D(const Client);
    rc<request::Response> rsp = UNWRAP(co_await d->session->get(q));
    co_return rsp;
}

auto Client::post(const request::Request& req,
                  std::string_view        body) const -> awaitable<Result<std::vector<byte>>> {
    C_D(const Client);
    // rc<std::string> csrf = m_csrf;
    rc<Response> rsp;
    EC_RET_CO(rsp, co_await d->session->post(req, asio::buffer(body)));

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

auto Client::prop(std::string_view name) const -> std::optional<std::any> {
    C_D(const Client);
    auto it = d->props.find(name);
    if (it != d->props.end()) {
        return it->second;
    }
    return std::nullopt;
}
void Client::set_prop(std::string_view name, std::any val) {
    C_D(Client);
    d->props.insert_or_assign(std::string(name), val);
}
void Client::save(const std::filesystem::path& p) {
    C_D(Client);
    d->session_share.save(p);
}
void Client::load(const std::filesystem::path& p) {
    C_D(Client);
    d->session_share.load(p);
}