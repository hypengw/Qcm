#pragma once

#include <ranges>
#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct SongDetail {
    std::vector<model::SongId> ids;
};
} // namespace params

namespace api_model
{

struct SongDetail {
    static Result<SongDetail> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<SongDetail>(bs).transform([](auto data) {
            auto n = std::min<usize>(data.songs.size(), data.privileges.size());
            for (usize i = 0; i < n; i++) {
                data.songs[i].privilege = data.privileges[i];
            }
            return data;
        });
    }

    std::vector<model::Song>            songs;
    std::vector<model::Song::Privilege> privileges;
    i64                                 code;
};
JSON_DEFINE(SongDetail);

} // namespace api_model

namespace api
{

struct SongDetail {
    using in_type                      = params::SongDetail;
    using out_type                     = api_model::SongDetail;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/v3/song/detail"s; };
    UrlParams   query() const { return {}; }
    Params      body() const {
        auto   view = std::views::transform(input.ids, [](auto& el) -> std::string {
            return fmt::format("{{\"id\": {}}}", el.as_str());
        });
        Params p;
        p["c"] = fmt::format("[{}]", fmt::join(view, ","));
        return p;
    }

    in_type input;
};
static_assert(ApiCP<SongDetail>);

} // namespace api

} // namespace ncm

/*
{
  "songs": [
    {
      "name": "伍佰的质朴疗愈：那就不要留，时光已过不再有",
      "id": 2640678150,
      "pst": 0,
      "t": 0,
      "ar": [
        {
          "id": 0,
          "name": "主播秋婉",
          "tns": [],
          "alias": []
        }
      ],
      "alia": [],
      "pop": 5.0,
      "st": 0,
      "rt": null,
      "fee": 0,
      "v": 2,
      "crbt": null,
      "cf": "",
      "al": {
        "id": 0,
        "name": "[DJ节目]主播秋婉的DJ节目 第18期",
        "picUrl": "https://p2.music.126.net/w6MnxmN0zADSqltRe7jPyQ==/109951170086037206.jpg",
        "tns": [],
        "pic_str": "109951170086037206",
        "pic": 109951170086037206
      },
      "dt": 2628284,
      "h": {
        "br": 320000,
        "fid": 0,
        "size": 105056923,
        "vd": 0.0,
        "sr": 44100
      },
      "m": null,
      "l": {
        "br": 320000,
        "fid": 0,
        "size": 105056923,
        "vd": 0.0,
        "sr": 44100
      },
      "sq": null,
      "hr": null,
      "a": null,
      "cd": "",
      "no": 0,
      "rtUrl": null,
      "ftype": 0,
      "rtUrls": [],
      "djId": 293850636,
      "copyright": 1,
      "s_id": 0,
      "mark": 0,
      "originCoverType": 0,
      "originSongSimpleData": null,
      "tagPicList": null,
      "resourceState": true,
      "version": 2,
      "songJumpInfo": null,
      "entertainmentTags": null,
      "awardTags": null,
      "single": 0,
      "noCopyrightRcmd": null,
      "mv": 0,
      "rtype": 0,
      "rurl": null,
      "mst": 9,
      "cp": 0,
      "publishTime": 1730046925615
    }
  ],
  "privileges": [
    {
      "id": 2640678150,
      "fee": 0,
      "payed": 0,
      "st": 0,
      "pl": 320000,
      "dl": 320000,
      "sp": 7,
      "cp": 1,
      "subp": 1,
      "cs": false,
      "maxbr": 320000,
      "fl": 320000,
      "toast": false,
      "flag": 0,
      "preSell": false,
      "playMaxbr": 0,
      "downloadMaxbr": 0,
      "maxBrLevel": null,
      "playMaxBrLevel": null,
      "downloadMaxBrLevel": null,
      "plLevel": null,
      "dlLevel": null,
      "flLevel": null,
      "rscl": null,
      "freeTrialPrivilege": {
        "resConsumable": false,
        "userConsumable": false,
        "listenType": null,
        "cannotListenReason": null,
        "playReason": null,
        "freeLimitTagType": null
      },
      "rightSource": 0,
      "chargeInfoList": null,
      "code": 0,
      "message": null
    }
  ],
  "code": 200
}
*/
