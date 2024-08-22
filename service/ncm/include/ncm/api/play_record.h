#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct PlayRecord {
    // Song, Album, Playlist, Djradio
    model::IdType type { model::IdType::Song };
    i64           limit { 100 };
};
} // namespace params

namespace model
{
struct PlayRecordItem {
    struct MultiTerminalInfo {
        std::string
            icon; //: "https://p6.music.126.net/obj/wonDlsKUwrLClGjCm8Kx/44266364630/cef0/cc7b/8a64/cdfe6c6231d20fb6666563450581183c.png",
        std::string os;     //"magicos",
        std::string osText; //"未知"
    };
    Time                playTime;
    std::string         resourceId;
    std::string         resourceType; // "SONG",
    std::optional<bool> banned;
    MultiTerminalInfo   multiTerminalInfo;

    struct Playlist {
        PlaylistId  id;
        std::string name;
        std::string coverImgUrl;
        Creator     creator;
        model::Song lastSong;
    };

    std::variant<Song, Album, Playlist, DjradioB> data;
};
JSON_DEFINE(PlayRecordItem);
JSON_DEFINE(PlayRecordItem::MultiTerminalInfo);
JSON_DEFINE(PlayRecordItem::Playlist);
} // namespace model

namespace api_model
{

struct PlayRecord {
    static Result<PlayRecord> parse(std::span<const byte> bs, const params::PlayRecord&);

    i64 code;
    struct Data {
        i64                                total;
        std::vector<model::PlayRecordItem> list;
    };
    Data data;
};
JSON_DEFINE(PlayRecord);
JSON_DEFINE(PlayRecord::Data);

} // namespace api_model

namespace api
{

struct PlayRecord {
    using in_type                      = params::PlayRecord;
    using out_type                     = api_model::PlayRecord;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const {
        return fmt::format("/play-record/{}/list", convert_from<std::string>(input.type));
    }
    UrlParams query() const { return {}; }
    Params    body() const {
        Params p;
        convert(p["limit"], input.limit);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<PlayRecord>);

} // namespace api

} // namespace ncm