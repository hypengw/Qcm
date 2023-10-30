#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct DjradioDetail {
    std::string id;
};
} // namespace params

namespace model
{

struct DjradioDetail {
    i64         id { 0 };
    std::string name;
    // dj	{…}
    i64         picId { 0 };
    std::string picUrl;
    std::string desc;
    i64         subCount { 0 };
    i64         shareCount { 0 };
    i64         likedCount { 0 };
    i64         programCount { 0 };
    i64         commentCount { 0 };
    model::Time createTime;
    i64         categoryId { 0 };
    std::string category;
    i64         secondCategoryId { 0 };
    std::string secondCategory;
    i64         radioFeeType { 0 };
    i64         feeScope { 0 };
    model::Time lastProgramCreateTime;
    i64         lastProgramId { 0 };
    // rcmdText	null
    bool subed { false };
    // commentDatas	[…]
    // feeInfo	null
    // unlockInfo	null
    bool original { false };
    i64  playCount { 0 };
    bool privacy { false };
    // bool disableShare	{false};
    // icon	null
    // activityInfo	null
    // toplistInfo	null
    bool dynamic { false };
    // labelDto	null
    // labels	null
    // detailRcmdTabOrpheus
    // "orpheus://rnpage?component=rn-podcast-voicelist-recmd&radioId=340108053" toast	null
};
JSON_DEFINE(DjradioDetail);

} // namespace model

namespace api_model
{

struct DjradioDetail {
    static Result<DjradioDetail> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<DjradioDetail>(bs);
    }

    model::DjradioDetail data;
};
JSON_DEFINE(DjradioDetail);

} // namespace api_model

namespace api
{

struct DjradioDetail {
    using in_type                      = params::DjradioDetail;
    using out_type                     = api_model::DjradioDetail;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/weapi/djradio/v2/get"; };
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["id"] = input.id;
        return p;
    }

    in_type input;
};
static_assert(ApiCP<DjradioDetail>);

} // namespace api

} // namespace ncm
