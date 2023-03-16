#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct QrcodeLogin {
    std::string key;
};
} // namespace params

namespace api_model
{
struct QrcodeLogin {
    static Result<QrcodeLogin> parse(std::span<const byte> bs) {
        ERROR_LOG("{}", bs);
        return api_model::parse_no_apierr<QrcodeLogin>(bs);
    }

    i64         code;
    std::string message;
    std::string nickname;
    std::string avatarUrl;
};
JSON_DEFINE(QrcodeLogin);

} // namespace api_model

namespace api
{

struct QrcodeLogin {
    using in_type                      = params::QrcodeLogin;
    using out_type                     = api_model::QrcodeLogin;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/login/qrcode/client/login"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["type"] = "1";
        p["key"]  = input.key;
        return p;
    }

    in_type input;
};
static_assert(ApiCP<QrcodeLogin>);

} // namespace api

} // namespace ncm
