#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/strv_helper.h"

namespace ncm
{

namespace params
{

struct UploadCloudInfo {
    std::string songId;
    std::string resourceId;

    std::string md5;
    std::string filename;
    std::string song;
    std::string album { "未知专辑" };
    std::string artist { "未知艺术家" };
    i64         bitrate;
};

} // namespace params

namespace api_model
{

struct UploadCloudInfo {
    static Result<UploadCloudInfo> parse(std::span<const byte> bs, const params::UploadCloudInfo&) {
        return api_model::parse<UploadCloudInfo>(bs);
    }

    i64         code;
    std::string songId;
};
JSON_DEFINE(UploadCloudInfo);

} // namespace api_model

namespace api
{

struct UploadCloudInfo {
    using in_type                      = params::UploadCloudInfo;
    using out_type                     = api_model::UploadCloudInfo;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/upload/cloud/info/v2"; };
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["songid"]     = input.songId;
        p["resourceId"] = input.resourceId;
        p["md5"]        = input.md5;
        p["song"]       = input.song;
        p["album"]      = input.album;
        p["artist"]     = input.artist;
        p["filename"]   = input.filename;
        p["bitrate"]    = convert_from<std::string>(input.bitrate);
        return p;
    }

    in_type input;
};
static_assert(ApiCP<UploadCloudInfo>);

} // namespace api

} // namespace ncm
