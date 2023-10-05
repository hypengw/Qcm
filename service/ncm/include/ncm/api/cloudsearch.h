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
    static Result<CloudSearch> parse(std::span<const byte> bs) {
        return api_model::parse<CloudSearch>(bs);
    }

    struct SongResult {
        std::vector<model::Song> songs;
        i64                      songCount;
    };
    struct AlbumResult {
        std::vector<model::Artist> albums;
        i64                        albumCount;
    };
    struct PlaylistResult {
        std::vector<model::Playlist> playlists;
        i64                          playlistCount;
    };
    struct ArtistResult {
        std::vector<model::Artist> artists;
        i64                        artistCount;
    };

    using SearchResult = std::variant<SongResult, AlbumResult, PlaylistResult, ArtistResult>;
    SearchResult result;
};
JSON_DEFINE(CloudSearch);
JSON_DEFINE(CloudSearch::SongResult);
JSON_DEFINE(CloudSearch::AlbumResult);
JSON_DEFINE(CloudSearch::PlaylistResult);
JSON_DEFINE(CloudSearch::ArtistResult);

} // namespace api_model

namespace api
{

struct CloudSearch {
    using in_type                      = params::CloudSearch;
    using out_type                     = api_model::CloudSearch;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::EAPI;

    std::string path() const { return "/eapi/cloudsearch/pc"; };
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["keywords"] = input.keywords;
        p["type"]     = To<std::string>::from(input.type);
        p["limit"]    = To<std::string>::from(input.limit);
        p["offset"]   = To<std::string>::from(input.offset);
        p["total"]    = To<std::string>::from(input.total);
        return p;
    }

    in_type input;
};
static_assert(ApiCP<CloudSearch>);

} // namespace api

} // namespace ncm
