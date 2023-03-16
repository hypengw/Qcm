#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct AlbumSublist {
    i64  offset { 0 };
    i64  limit { 25 };
    bool total { true };
};
} // namespace params

namespace model
{
struct AlbumSublistItem {
    // i64 picId;
    // "msg" : [],
    i64                        subTime;
    std::vector<std::string>   alias;
    std::vector<model::Artist> artists;
    std::string                picUrl;
    std::string                name;
    i64                        id;
    i64                        size;
    std::vector<std::string>   transNames;
};
} // namespace model

namespace api_model
{

struct AlbumSublist {
    static Result<AlbumSublist> parse(std::span<const byte> bs) {
        return api_model::parse<AlbumSublist>(bs);
    }

    i64                                  code;
    i64                                  count;
    bool                                 hasMore;
    std::vector<model::AlbumSublistItem> data;
};
JSON_DEFINE(AlbumSublist);

} // namespace api_model

namespace api
{

struct AlbumSublist {
    using in_type                      = params::AlbumSublist;
    using out_type                     = api_model::AlbumSublist;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/album/sublist"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["offset"] = To<std::string>::from(input.offset);
        p["limit"]  = To<std::string>::from(input.limit);
        p["total"]  = To<std::string>::from(input.total);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<AlbumSublist>);

} // namespace api

} // namespace ncm

/*
        {
            "subTime": xxxxxxx,
            "msg": [],
            "alias": [ "xxxxx" ],
            "artists": [
                {
                    "img1v1Id": 18686200114669622,
                    "topicPerson": 0,
                    "alias": [],
                    "picId": 0,
                    "briefDesc": "",
                    "musicSize": 0,
                    "albumSize": 0,
                    "picUrl":
   "https://p2.music.126.net/6y-UleORITEDbvrOLV0Q8A==/5639395138885805.jpg", "img1v1Url":
   "https://p2.music.126.net/VnZiScyynLG7atLIZ2YPkw==/18686200114669622.jpg", "followed": false,
                    "trans": "",
                    "name": "刀郎",
                    "id": 2517,
                    "img1v1Id_str": "18686200114669622"
                }
            ],
            "picId": 109951163281535703,
            "picUrl": "https://p2.music.126.net/dCMVhVKVSVi9LQ6cBp_rTg==/109951163281535703.jpg",
            "name": "2002年的第一场雪",
            "id": 7607,
            "size": 12,
            "transNames": [
                "xxxxx"
            ]
        },
*/
