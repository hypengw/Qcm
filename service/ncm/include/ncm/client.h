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
    auto perform(const TApi& api) -> awaitable<Result<typename TApi::out_type>> {
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

        if (! res.has_value()) {
            out = nstd::unexpected(res.error());
        } else {
            out = out_type::parse(std::move(res).value(), api.input);
        }
        co_return out.map_error([&api](auto err) {
            return Error::push(err, api::format_api(api.path(), api.query(), api.body()));
        });
    }

    template<typename Fn, typename H>
        requires std::invocable<Fn> || helper::is_awaitable<Fn>
    void spawn(Fn&& t, H&& h = asio::detached) {
        asio::co_spawn(get_executor(), std::forward<Fn>(t), std::move(h));
    }

    executor_type& get_executor();

    auto rsp(const request::Request&) const -> awaitable<rc<request::Response>>;

    template<api::CryptoType CT>
    auto make_req(std::string_view url, const UrlParams&) const -> request::Request;

    template<api::CryptoType CT>
    auto encrypt(std::string_view path, const Params&) -> std::optional<std::string>;

private:
    auto post(const request::Request&, std::string_view) -> awaitable<Result<std::vector<byte>>>;

    rc<request::Session> m_session;
    rc<std::string>      m_csrf;
    rc<Crypto>           m_crypto;

    rc<executor_type>    m_ex;
    rc<request::Request> m_req_common;
};

} // namespace ncm
