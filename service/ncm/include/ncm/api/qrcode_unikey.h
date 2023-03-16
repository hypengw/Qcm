#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct QrcodeUnikey {};
} // namespace params

namespace api_model
{
struct QrcodeUnikey {
    static Result<QrcodeUnikey> parse(std::span<const byte> bs) {
        ERROR_LOG("{}", bs);
        return api_model::parse<QrcodeUnikey>(bs).map([](auto in) {
            in.qrurl = fmt::format("https://music.163.com/login?codekey={}", in.unikey);
            return in;
        });
    }

    i64         code;
    std::string unikey;
    std::string qrurl;
};
JSON_DEFINE(QrcodeUnikey);

} // namespace api_model

namespace api
{

struct QrcodeUnikey {
    using in_type                      = params::QrcodeUnikey;
    using out_type                     = api_model::QrcodeUnikey;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/login/qrcode/unikey"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["type"] = "1";
        return p;
    }

    in_type input;
};
static_assert(ApiCP<QrcodeUnikey>);

} // namespace api

} // namespace ncm
