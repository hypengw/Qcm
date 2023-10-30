#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct DjradioProgram {
    std::string radioId;
    i64         limit { 60 };
    i64         offset { 0 };
    bool        asc { false };
};
} // namespace params

namespace api_model
{

struct DjradioProgram {
    static Result<DjradioProgram> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<DjradioProgram>(bs);
    }

    i64                         count { 0 };
    bool                        more { false };
    std::vector<model::Program> programs;
};
JSON_DEFINE(DjradioProgram);

} // namespace api_model

namespace api
{

struct DjradioProgram {
    using in_type                      = params::DjradioProgram;
    using out_type                     = api_model::DjradioProgram;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/weapi/dj/program/byradio"; };
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["radioId"] = input.radioId;
        convert(p["offset"], input.offset);
        convert(p["limit"], input.limit);
        convert(p["asc"], input.asc);
        return p;
    }

    in_type input;
};
static_assert(ApiCP<DjradioProgram>);

} // namespace api

} // namespace ncm
