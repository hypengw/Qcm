#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/strv_helper.h"

namespace ncm
{

namespace params
{
struct RadioGet {
    enum class Mode
    {
        DEFAULT = 0,
        AIDJ,
        FAMILIAR,
        EXPLORE,
        SCENE_RCMD
    };
    enum class SubMode
    {
        NONE,
        EXERCISE,
        FOCUS,
        NIGHT_EMO
    };
    Mode    mode { Mode::DEFAULT };
    SubMode subMode { SubMode::NONE };
    i64     limit { 0 };
};
} // namespace params
} // namespace ncm

DECLARE_CONVERT(std::string_view, ncm::params::RadioGet::Mode)
DECLARE_CONVERT(std::string_view, ncm::params::RadioGet::SubMode)

namespace ncm
{
namespace api_model
{

struct RadioGet {
    static Result<RadioGet> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<RadioGet>(bs);
    }

    i64                       code;
    bool                      popAdjust;
    std::vector<model::SongB> data;
    //  "tag": null,
};
JSON_DEFINE(RadioGet);

} // namespace api_model

namespace api
{

struct RadioGet {
    using in_type                      = params::RadioGet;
    using out_type                     = api_model::RadioGet;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/radio/get"s; }
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["mode"] = convert_from<std::string_view>(input.mode);
        if (input.subMode != params::RadioGet::SubMode::NONE) {
            p["subMode"] = convert_from<std::string_view>(input.mode);
        }
        if (input.limit > 0) convert(p["limit"], input.limit);
        return p;
    }

    in_type input;
};
static_assert(ApiCP<RadioGet>);

} // namespace api

} // namespace ncm
