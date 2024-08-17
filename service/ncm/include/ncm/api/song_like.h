#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct SongLike {
    model::SongId uid;
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct SongLike {};
} // namespace model

namespace api_model
{

struct SongLike {
    static Result<SongLike> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<SongLike>(bs);
    }
    // 200
    i64              code;
    model::Time      checkPoint;
    std::vector<i64> ids;
};
JSON_DEFINE(SongLike);

} // namespace api_model

namespace api
{

struct SongLike {
    using in_type                      = params::SongLike;
    using out_type                     = api_model::SongLike;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/song/like/get"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["uid"] = input.uid.as_str();
        return p;
    }
    in_type input;
};
static_assert(ApiCP<SongLike>);

} // namespace api

} // namespace ncm
