#pragma once

#include <asio/detached.hpp>
#include <asio/co_spawn.hpp>
#include <asio/strand.hpp>

#include "asio_helper/helper.h"
#include "request/session.h"
#include "request/request.h"
#include "ncm/type.h"
#include "ncm/api.h"

namespace ncm
{

constexpr std::string_view BASE_URL = "https://music.163.com";
using bytes_view                    = std::span<const byte>;

class Crypto;
class Client {
public:
    using executor_type = asio::thread_pool::executor_type;
    Client(rc<request::Session> session, executor_type ex, std::string device_id);
    ~Client();

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

        auto        url  = format_url<TApi::crypto>(base_url, api.path());
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

    template<api::CryptoType CT>
    auto format_url(std::string_view base, std::string_view path) -> std::string;

private:
    auto post(const request::Request&, std::string_view) -> awaitable<Result<std::vector<byte>>>;

    class Private;
    rc<Private> d_ptr;
    C_DECLARE_PRIVATE_FUNC(Client, d_ptr);
};

} // namespace ncm
