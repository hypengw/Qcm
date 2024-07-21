#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct UserCloud {
    i64 offset { 0 };
    i64 limit { 30 };
};
} // namespace params

namespace model
{
struct UserCloudItem {
    Song        simpleSong;
    std::string coverId;
    std::string lyricId;
    std::string album;
    std::string artist;
    i64         bitrate { 0 };
    SongId      songId;
    Time        addTime;
    std::string songName;
    i64         cover { 0 };
    i64         version { 0 };
    i64         fileSize { 0 };
    std::string fileName;
};
JSON_DEFINE(UserCloudItem);

} // namespace model

namespace api_model
{

struct UserCloud {
    static Result<UserCloud> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<UserCloud>(bs);
    }

    std::vector<model::UserCloudItem> data;
    i64                               count { 0 };
    std::string                       size;
    std::string                       maxSize;
    i64                               upgradeSign { 0 };
    bool                              hasMore { false };
};
JSON_DEFINE(UserCloud);

} // namespace api_model

namespace api
{

struct UserCloud {
    using in_type                      = params::UserCloud;
    using out_type                     = api_model::UserCloud;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/v1/cloud/get"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        convert(p["offset"], input.offset);
        convert(p["limit"], input.limit);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<UserCloud>);

} // namespace api

} // namespace ncm