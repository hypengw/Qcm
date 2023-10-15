#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct AlbumDetailDynamic {
    std::string id;
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace api_model
{

struct AlbumDetailDynamic {
    static Result<AlbumDetailDynamic> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<AlbumDetailDynamic>(bs);
    }
    i64  code;
    bool onSale;
    // albumGameInfo	null
    i64         commentCount;
    i64         likedCount;
    i64         shareCount;
    bool        isSub;
    model::Time subTime;
    i64         subCount;
};
JSON_DEFINE(AlbumDetailDynamic);

} // namespace api_model

namespace api
{

struct AlbumDetailDynamic {
    using in_type                      = params::AlbumDetailDynamic;
    using out_type                     = api_model::AlbumDetailDynamic;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/album/detail/dynamic"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["id"] = input.id;
        return p;
    }
    in_type input;
};
static_assert(ApiCP<AlbumDetailDynamic>);

} // namespace api

} // namespace ncm
