#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct Logout {};
} // namespace params

namespace api_model
{
struct Logout {
    static Result<Logout> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse_no_apierr<Logout>(bs);
    }

    i64 code;
};
JSON_DEFINE(Logout);

} // namespace api_model

namespace api
{

struct Logout {
    using in_type                      = params::Logout;
    using out_type                     = api_model::Logout;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/logout"; }
    UrlParams        query() const { return {}; }
    Params           body() const { return {}; }
    in_type          input;
};
static_assert(ApiCP<Logout>);

} // namespace api

} // namespace ncm
