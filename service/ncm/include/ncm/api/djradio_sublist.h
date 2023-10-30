#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct DjradioSublist {
    i64  offset { 0 };
    i64  limit { 30 };
    bool total { true };
};
} // namespace params

namespace api_model
{

struct DjradioSublist {
    static Result<DjradioSublist> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<DjradioSublist>(bs);
    }

    i64                         count { 0 };
    model::Time                 time;
    bool                        hasMore { false };
    std::vector<model::Djradio> djRadios;
};
JSON_DEFINE(DjradioSublist);

} // namespace api_model

namespace api
{

struct DjradioSublist {
    using in_type                      = params::DjradioSublist;
    using out_type                     = api_model::DjradioSublist;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/djradio/get/subed"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        convert(p["offset"], input.offset);
        convert(p["limit"], input.limit);
        convert(p["total"], input.total);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<DjradioSublist>);

} // namespace api

} // namespace ncm

/*
id	792544462
name	"四只烤翅"
picUrl	"https://p2.music.126.net…=/109951163773467863.jpg"
programCount	35
subCount	272656
createTime	1546664883212
categoryId	2001
category	"创作翻唱"
rcmdtext	"四合院现场，粗粝而动人"
radioFeeType	0
feeScope	0
playCount	196385552
subed	false
dj	{…}
copywriter	"四合院现场，粗粝而动人"
buyed	false
*/
