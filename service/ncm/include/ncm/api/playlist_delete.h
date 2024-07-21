#pragma once

#include <ranges>

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct PlaylistDelete {
    std::vector<model::PlaylistId> ids;
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct PlaylistDelete {};
} // namespace model

namespace api_model
{

struct PlaylistDelete {
    static Result<PlaylistDelete> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<PlaylistDelete>(bs);
    }
    // 200
    i64 code;
};
JSON_DEFINE(PlaylistDelete);

} // namespace api_model

namespace api
{

struct PlaylistDelete {
    using in_type                      = params::PlaylistDelete;
    using out_type                     = api_model::PlaylistDelete;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/playlist/remove"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["ids"] = fmt::format("[{}]", fmt::join(model::id_str_range_view(input.ids), ","));
        return p;
    }
    in_type input;
};
static_assert(ApiCP<PlaylistDelete>);

} // namespace api

} // namespace ncm
