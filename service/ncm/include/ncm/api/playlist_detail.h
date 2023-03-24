#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct PlaylistDetail {
    std::string id;
    i64         n { 1000 };
    i64         s { 8 }; // 歌单最近的 s 个收藏者
};
} // namespace params

/*
specialType 说明
0           普通歌单
5           红心歌单
10          置顶歌单
20          尾部歌单
100         官方歌单
200         视频歌单
300         分享歌单
*/

namespace api_model
{

struct PlaylistDetail {
    static Result<PlaylistDetail> parse(std::span<const byte> bs) {
        return api_model::parse<PlaylistDetail>(bs).map([](PlaylistDetail in) {
            if (in.playlist.tracks && in.privileges) {
                auto& tracks     = in.playlist.tracks.value();
                auto& privileges = in.privileges.value();
                auto  len        = std::min(tracks.size(), privileges.size());
                for (usize i = 0; i < len; i++) {
                    tracks[i].privilege = privileges[i];
                }
            }
            return in;
        });
    }

    i64                                                code;
    model::Playlist                                    playlist;
    std::optional<std::vector<model::Song::Privilege>> privileges;
};
JSON_DEFINE(PlaylistDetail);

} // namespace api_model

namespace api
{

struct PlaylistDetail {
    using in_type                      = params::PlaylistDetail;
    using out_type                     = api_model::PlaylistDetail;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::EAPI;

    std::string_view path() const { return "/eapi/v6/playlist/detail"; };
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["id"] = input.id;
        p["n"]  = To<std::string>::from(input.n);
        p["s"]  = To<std::string>::from(input.s);
        return p;
    }

    in_type input;
};
static_assert(ApiCP<PlaylistDetail>);

} // namespace api

} // namespace ncm
