#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/strv_helper.h"

namespace ncm
{

namespace params
{

struct CloudPub {
    std::string songId;
};

} // namespace params

namespace api_model
{

struct CloudPub {
    static Result<CloudPub> parse(std::span<const byte> bs, const params::CloudPub&) {
        return api_model::parse<CloudPub>(bs);
    }

    i64 code;
};
JSON_DEFINE(CloudPub);

} // namespace api_model

namespace api
{

struct CloudPub {
    using in_type                      = params::CloudPub;
    using out_type                     = api_model::CloudPub;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/cloud/pub/v2"; };
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["songid"]   = input.songId;
        return p;
    }

    in_type input;
};
static_assert(ApiCP<CloudPub>);

} // namespace api

} // namespace ncm
