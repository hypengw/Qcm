#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct RecommendResource {};
} // namespace params

namespace model
{
struct RecommendResourceItem {
    i64         id;
    i64         type;
    std::string name;
    std::string copywriter;
    std::string picUrl;
    i64         playcount;
    Time        createTime;
    //      "creator": null,
    i64 trackCount;
};
} // namespace model

namespace api_model
{

struct RecommendResource {
    static Result<RecommendResource> parse(std::span<const byte> bs) {
        return api_model::parse<RecommendResource>(bs);
    }
    std::vector<model::RecommendResourceItem> recommend;
    i64                                       code;
};
JSON_DEFINE(RecommendResource);

} // namespace api_model

namespace api
{

struct RecommendResource {
    using in_type                      = params::RecommendResource;
    using out_type                     = api_model::RecommendResource;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/v1/discovery/recommend/resource"; }
    UrlParams        query() const { return {}; }
    Params           body() const { return {}; }
    in_type          input;
};
static_assert(ApiCP<RecommendResource>);

} // namespace api

} // namespace ncm
