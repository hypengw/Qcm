#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct RecommendSongs {};
} // namespace params

namespace api_model
{

struct RecommendSongs {
    static Result<RecommendSongs> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<RecommendSongs>(bs);
    }

    struct Data {
        std::vector<model::Song> dailySongs;
    };

    Data data;
    i64 code;
};
JSON_DEFINE(RecommendSongs);

} // namespace api_model

namespace api
{

struct RecommendSongs {
    using in_type                      = params::RecommendSongs;
    using out_type                     = api_model::RecommendSongs;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/v3/discovery/recommend/songs"; }
    UrlParams        query() const { return {}; }
    Params           body() const { return {}; }
    in_type          input;
};
static_assert(ApiCP<RecommendSongs>);

} // namespace api

} // namespace ncm
