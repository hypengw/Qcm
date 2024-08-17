#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{

struct CloudUploadCheck {
    std::string bitrate;
    std::string ext;
    usize       length;
    std::string md5;
    std::string songId;
    i64         version;
};

} // namespace params

namespace api_model
{

struct CloudUploadCheck {
    static Result<CloudUploadCheck> parse(std::span<const byte> bs,
                                          const params::CloudUploadCheck&) {
        return api_model::parse<CloudUploadCheck>(bs);
    }

    i64 code;
};
JSON_DEFINE(CloudUploadCheck);

} // namespace api_model

namespace api
{

struct CloudUploadCheck {
    using in_type                      = params::CloudUploadCheck;
    using out_type                     = api_model::CloudUploadCheck;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/weapi/cloud/upload/check"; };
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["bitrate"] = input.bitrate;
        p["ext"]     = input.ext;
        convert(p["length"], input.length);
        p["md5"]    = input.md5;
        p["songId"] = input.songId;
        convert(p["version"], input.version);
        return p;
    }

    in_type input;
};
static_assert(ApiCP<CloudUploadCheck>);

} // namespace api

} // namespace ncm
