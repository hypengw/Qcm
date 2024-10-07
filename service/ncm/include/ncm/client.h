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
    auto get_base(const TApi& api) const {
        if constexpr (api::ApiCP_Base<TApi>) {
            return TApi::base;
        } else if constexpr (api::ApiCP_BaseFunc<TApi>) {
            return api.base();
        } else {
            return BASE_URL;
        }
    }

    template<typename TApi>
        requires api::ApiCP<TApi>
    auto prepare_body(const TApi& api, request::Request& req) {
        if constexpr (api::ApiCP_Reader<TApi>) {
            req.set_opt(api.body());
            return ""sv;
        } else {
            return UNWRAP(encrypt<TApi::crypto>(api.path(), api.body()));
        }
    }

    template<typename TApi>
        requires api::ApiCP<TApi>
    auto perform(const TApi& api, u32 timeout = 60) -> awaitable<Result<typename TApi::out_type>> {
        using out_type = typename TApi::out_type;

        auto base_url = get_base(api);
        auto url      = format_url<TApi::crypto>(base_url, api.path());
        auto req      = make_req<TApi::crypto>(url, api.query());

        if constexpr (api::ApiCP_Header<TApi>) {
            req.update_header(api.header());
        }
        req.template get_opt<request::req_opt::Timeout>().set_transfer_timeout(timeout);

        auto                      body = prepare_body(api, req);
        Result<std::vector<byte>> res  = co_await post(req, body);

        co_return res
            .and_then([&api](const auto& res) {
                return out_type::parse(res, api.input);
            })
            .map_error([&api](auto err) {
                return Error::push(err, api::format_api(api.path(), api.query(), api.body()));
            });
    }

    executor_type& get_executor();

    auto rsp(const request::Request&) const -> awaitable<rc<request::Response>>;

    template<api::CryptoType CT>
    auto make_req(std::string_view url, const UrlParams&) const -> request::Request;

    template<api::CryptoType CT>
    auto encrypt(std::string_view path, const Params&) -> std::optional<std::string>;

    template<api::CryptoType CT>
    auto format_url(std::string_view base, std::string_view path) const -> std::string;

    auto prop(std::string_view) const -> std::optional<std::any>;
    void set_prop(std::string_view, std::any);

    void save(const std::filesystem::path&);
    void load(const std::filesystem::path&);

private:
    auto post(const request::Request&,
              std::string_view) const -> awaitable<Result<std::vector<byte>>>;

    class Private;
    rc<Private> d_ptr;
    C_DECLARE_PRIVATE_FUNC(Client, d_ptr);
};

} // namespace ncm
