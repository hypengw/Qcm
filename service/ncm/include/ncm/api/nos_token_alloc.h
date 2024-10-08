#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/strv_helper.h"

namespace ncm
{

namespace params
{

struct NosTokenAlloc {
    static constexpr auto bucket_private_cloud { "jd-musicrep-privatecloud-audio-public"sv };

    std::string bucket;
    std::string ext;
    std::string filename;
    bool        local { false };
    i64         nos_product { 3 };
    std::string type { "audio" };
    std::string md5;
};

} // namespace params

namespace api_model
{

struct NosTokenAlloc {
    static Result<NosTokenAlloc> parse(std::span<const byte> bs, const params::NosTokenAlloc&) {
        return api_model::parse<NosTokenAlloc>(bs);
    }

    i64 code;
    struct Result_ {
        std::string objectKey;
        std::string token;
        i64         resourceId;
    };
    Result_ result;
};
JSON_DEFINE(NosTokenAlloc);
JSON_DEFINE(NosTokenAlloc::Result_);

} // namespace api_model

namespace api
{

struct NosTokenAlloc {
    using in_type                      = params::NosTokenAlloc;
    using out_type                     = api_model::NosTokenAlloc;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/nos/token/alloc"; };
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["bucket"]   = input.bucket;
        p["ext"]      = input.ext;
        p["md5"]      = input.md5;
        p["filename"] = input.filename;
        convert(p["nos_product"], input.nos_product);
        convert(p["local"], input.local);
        p["type"] = input.type;
        return p;
    }

    in_type input;
};
static_assert(ApiCP<NosTokenAlloc>);

} // namespace api

} // namespace ncm
