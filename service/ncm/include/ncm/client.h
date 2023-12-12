#pragma once

#include <asio/detached.hpp>
#include <asio/co_spawn.hpp>
#include <asio/strand.hpp>

#include "asio_helper/helper.h"
#include "request/session.h"
#include "request/request.h"
#include "ncm/type.h"
#include "ncm/model.h"
#include "ncm/api.h"

namespace ncm
{

constexpr std::string_view BASE_URL = "https://music.163.com";
using bytes_view                    = std::span<const byte>;

class Crypto;
class Client {
public:
    Client(rc<request::Session>, asio::any_io_executor);
    ~Client();
    using executor_type = asio::any_io_executor;

    template<typename TApi>
        requires api::ApiCP<TApi>
    awaitable<Result<typename TApi::out_type>> perform(const TApi& api) {
        using out_type = typename TApi::out_type;
        Result<out_type> out;
        std::string_view base_url;

        if constexpr (api::ApiCP_Base<TApi>)
            base_url = TApi::base;
        else
            base_url = BASE_URL;

        auto        url  = fmt::format("{}{}", base_url, api.path());
        auto        req  = make_req<TApi::crypto>(url, api.query());
        std::string body = UNWRAP(encrypt<TApi::crypto>(api.path(), api.body()));

        Result<std::vector<byte>> res = co_await post(req, body);

        if (! res.has_value()) co_return nstd::unexpected(res.error());
        co_return out_type::parse(std::move(res).value(), api.input);
    }

    template<typename Fn, typename H>
        requires std::invocable<Fn> || helper::is_awaitable<Fn>
    void spawn(Fn&& t, H&& h = asio::detached) {
        asio::co_spawn(get_executor(), std::forward<Fn>(t), std::move(h));
    }

    executor_type& get_executor();

    awaitable<rc<request::Response>> rsp(const request::Request&) const;

    template<api::CryptoType CT>
    request::Request make_req(std::string_view url, const UrlParams&) const;

    template<api::CryptoType CT>
    std::optional<std::string> encrypt(std::string_view path, const Params&);

    void set_proxy(const request::req_opt::Proxy&);

private:
    awaitable<Result<std::vector<byte>>> post(const request::Request&, std::string_view);

    rc<request::Session> m_session;
    rc<std::string>      m_csrf;
    rc<Crypto>           m_crypto;

    rc<executor_type>    m_ex;
    rc<request::Request> m_req_common;
};

} // namespace ncm
