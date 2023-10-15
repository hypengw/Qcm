#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct PlaylistList {
    std::string cat;
    std::string order { "hot" }; // hot,new
    i64         limit { 50 };
    i64         offset { 0 };
    bool        total { true };
};
} // namespace params

namespace model
{
struct PlaylistList {};
} // namespace model

namespace api_model
{

struct PlaylistList {
    static Result<PlaylistList> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<PlaylistList>(bs);
    }

    i64                          code;
    std::vector<model::Playlist> playlists;
    i64                          total;
    bool                         more;
    std::string                  cat;
};
JSON_DEFINE(PlaylistList);

} // namespace api_model

namespace api
{

struct PlaylistList {
    using in_type                      = params::PlaylistList;
    using out_type                     = api_model::PlaylistList;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/playlist/list"; };
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["cat"]    = input.cat;
        p["order"]  = input.order;
        p["limit"]  = To<std::string>::from(input.limit);
        p["offset"] = To<std::string>::from(input.offset);
        p["total"]  = To<std::string>::from(input.total);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<PlaylistList>);

} // namespace api

} // namespace ncm
