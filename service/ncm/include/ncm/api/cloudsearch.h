#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{

struct CloudSearch {
    std::string keywords;
    i32 type { 1 }; // 1: 单曲, 10: 专辑, 100: 歌手, 1000: 歌单, 1002: 用户, 1004: MV, 1006: 歌词,
                    // 1009: 电台, 1014: 视频
    i32  limit { 30 };
    i32  offset { 0 };
    bool total { true };
};

} // namespace params

namespace api_model
{

struct CloudSearch {
    static Result<CloudSearch> parse(std::span<const byte> bs, const params::CloudSearch&) {
        return api_model::parse<CloudSearch>(bs);
    }

    struct SongResult {
        std::optional<std::vector<model::Song>> songs;
        i64                                     songCount;
    };
    struct AlbumResult {
        std::optional<std::vector<model::Album>> albums;
        i64                                      albumCount;
    };
    struct PlaylistResult {
        std::optional<std::vector<model::Playlist>> playlists;
        i64                                         playlistCount;
    };
    struct ArtistResult {
        std::optional<std::vector<model::Artist>> artists;
        i64                                       artistCount;
    };

    struct DjradioResult {
        std::optional<std::vector<model::Djradio>> djRadios;
        i64                                       djRadiosCount;
    };

    using SearchResult = std::variant<SongResult, AlbumResult, PlaylistResult, ArtistResult, DjradioResult>;
    SearchResult result;
};
JSON_DEFINE(CloudSearch);
JSON_DEFINE(CloudSearch::SongResult);
JSON_DEFINE(CloudSearch::AlbumResult);
JSON_DEFINE(CloudSearch::PlaylistResult);
JSON_DEFINE(CloudSearch::ArtistResult);
JSON_DEFINE(CloudSearch::DjradioResult);

} // namespace api_model

namespace api
{

struct CloudSearch {
    using in_type                      = params::CloudSearch;
    using out_type                     = api_model::CloudSearch;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/weapi/cloudsearch/pc"; };
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["s"]      = input.keywords;
        convert(p["type"], input.type);
        convert(p["limit"], input.limit);
        convert(p["offset"], input.offset);
        convert(p["total"], input.total);
        return p;
    }

    in_type input;
};
static_assert(ApiCP<CloudSearch>);

} // namespace api

} // namespace ncm
