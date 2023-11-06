#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{

struct PlaylistTracks {
    constexpr static std::array oper_strs { "add", "del" };
    enum class Oper
    {
        Add = 0,
        Del = 1
    };

    Oper                     op { Oper::Add }; // del
    std::string              pid;
    std::vector<std::string> trackIds;
    bool                     imme { true };
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

struct PlaylistTracks {
    static Result<PlaylistTracks> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<PlaylistTracks>(bs);
    }
    // 200
    i64 code;
};
JSON_DEFINE(PlaylistTracks);

} // namespace api_model

namespace api
{

struct PlaylistTracks {
    using in_type                      = params::PlaylistTracks;
    using out_type                     = api_model::PlaylistTracks;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/playlist/manipulate/tracks"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["op"]       = params::PlaylistTracks::oper_strs[(int)input.op % 2];
        p["pid"]      = input.pid;
        p["trackIds"] = fmt::format("[{}]", fmt::join(input.trackIds, ","));
        convert(p["imme"], input.imme);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<PlaylistTracks>);

} // namespace api

} // namespace ncm
