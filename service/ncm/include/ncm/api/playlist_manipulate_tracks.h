#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{

struct PlaylistManipulateTracks {
    constexpr static std::array oper_strs { "add", "del" };
    enum class Oper
    {
        Add = 0,
        Del = 1
    };

    Oper                       op { Oper::Add }; // del
    model::PlaylistId          pid;
    std::vector<model::SongId> trackIds;
    bool                       imme { true };
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct PlaylistTracks {};
} // namespace model

namespace api_model
{

struct PlaylistManipulateTracks {
    static Result<PlaylistManipulateTracks> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<PlaylistManipulateTracks>(bs);
    }
    // 200
    i64 code;
};
JSON_DEFINE(PlaylistManipulateTracks);

} // namespace api_model

namespace api
{

struct PlaylistManipulateTracks {
    using in_type                      = params::PlaylistManipulateTracks;
    using out_type                     = api_model::PlaylistManipulateTracks;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/playlist/manipulate/tracks"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["op"]  = params::PlaylistManipulateTracks::oper_strs[(int)input.op % 2];
        p["pid"] = input.pid.as_str();
        p["trackIds"] =
            fmt::format("[{}]", fmt::join(model::id_str_range_view(input.trackIds), ","));
        convert(p["imme"], input.imme);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<PlaylistManipulateTracks>);

} // namespace api

} // namespace ncm
