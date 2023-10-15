#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct RadioLike {
    std::string alg { "itembased" };
    i64         time { 3 };
    std::string trackId;
    bool        like;
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct RadioLike {};
} // namespace model

namespace api_model
{

struct RadioLike {
    static Result<RadioLike> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<RadioLike>(bs);
    }
    // 200
    i64 code;
};
JSON_DEFINE(RadioLike);

} // namespace api_model

namespace api
{

struct RadioLike {
    using in_type                      = params::RadioLike;
    using out_type                     = api_model::RadioLike;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::EAPI;

    std::string_view path() const { return "/eapi/radio/like"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["alg"]     = input.alg;
        p["time"]    = To<std::string>::from(input.time);
        p["trackId"] = input.trackId;
        p["like"]    = To<std::string>::from(input.like);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<RadioLike>);

} // namespace api

} // namespace ncm
