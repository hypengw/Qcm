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

namespace model
{
struct PlaylistDetail {
    i64         id;
    std::string name;
    // coverImgId	109951167805071570
    std::string coverImgUrl;
    // coverImgId_str	"109951167805071571"
    // adType	0
    i64  userId;
    Time createTime;
    i64  status;
    // opRecommend	false
    // highQuality	false
    // newImported	false
    Time updateTime;
    i64  trackCount;
    i64  specialType;
    // privacy	0
    // trackUpdateTime	1678022963095
    std::string commentThreadId;
    i64         playCount;
    // trackNumberUpdateTime	1678018138230
    // subscribedCount	4
    // cloudTrackCount	1
    // ordered	true
    std::optional<std::string> description;
    std::vector<std::string>   tags;
    // updateFrequency	null
    // backgroundCoverId	0
    // backgroundCoverUrl	null
    // titleImage	0
    // titleImageUrl	null
    // englishTitle	null
    // officialPlaylistType	null
    // copied	false
    // relateResType	null
    // subscribers	[…]
    bool subscribed;
    // creator	{…}
    std::vector<Song> tracks;
    // videoIds	null
    // videos	null
    // trackIds	[…]
    // bannedTrackIds	null
    i64 shareCount;
    i64 commentCount;
    // remixVideo	null
    // sharedUsers	null
    // historySharedUsers	null
    // gradeStatus	"NONE"
    // score	null
    // algTags	null
};

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

} // namespace model

namespace api_model
{

struct PlaylistDetail {
    static Result<PlaylistDetail> parse(std::span<const byte> bs) {
        return api_model::parse<PlaylistDetail>(bs).map([](PlaylistDetail in) {
            auto& tracks = in.playlist.tracks;
            if (in.privileges) {
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
    model::PlaylistDetail                              playlist;
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
