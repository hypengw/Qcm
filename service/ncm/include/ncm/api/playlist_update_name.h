#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct PlaylistUpdateName {
    model::PlaylistId id;
    std::string       name;
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct PlaylistUpdateName {};
} // namespace model

namespace api_model
{

struct PlaylistUpdateName {
    static Result<PlaylistUpdateName> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<PlaylistUpdateName>(bs);
    }
    i64 code;
};
JSON_DEFINE(PlaylistUpdateName);

} // namespace api_model

namespace api
{

struct PlaylistUpdateName {
    using in_type                      = params::PlaylistUpdateName;
    using out_type                     = api_model::PlaylistUpdateName;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/playlist/update/name"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["id"]   = input.id.as_str();
        p["name"] = input.name;
        return p;
    }
    in_type input;
};
static_assert(ApiCP<PlaylistUpdateName>);

} // namespace api

} // namespace ncm
