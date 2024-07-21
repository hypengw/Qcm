#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct PlaylistDetailDynamic {
    model::PlaylistId id;
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace api_model
{

struct PlaylistDetailDynamic {
    static Result<PlaylistDetailDynamic> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<PlaylistDetailDynamic>(bs);
    }
    i64  code;
    i64  commentCount;
    i64  bookedCount;
    i64  shareCount;
    i64  playCount;
    bool subscribed;
    bool followed;
};
JSON_DEFINE(PlaylistDetailDynamic);

} // namespace api_model

namespace api
{

struct PlaylistDetailDynamic {
    using in_type                      = params::PlaylistDetailDynamic;
    using out_type                     = api_model::PlaylistDetailDynamic;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/playlist/detail/dynamic"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["id"] = input.id.as_str();
        return p;
    }
    in_type input;
};
static_assert(ApiCP<PlaylistDetailDynamic>);

} // namespace api

} // namespace ncm
