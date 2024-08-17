#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct PlaylistCreate {
    enum class Privacy
    {
        Public  = 0,
        Private = 10
    };
    std::string name;
    Privacy     privacy { Privacy::Public };
    std::string type { "NORMAL" }; // NORMAL|VIDEO|SHARED
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct PlaylistCreate {};
} // namespace model

namespace api_model
{

struct PlaylistCreate {
    static Result<PlaylistCreate> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<PlaylistCreate>(bs);
    }
    i64             id { 0 };
    model::Playlist playlist;
};
JSON_DEFINE(PlaylistCreate);

} // namespace api_model

namespace api
{

struct PlaylistCreate {
    using in_type                      = params::PlaylistCreate;
    using out_type                     = api_model::PlaylistCreate;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::EAPI;

    std::string_view path() const { return "/playlist/create"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        convert(p["privacy"], (int)input.privacy);
        p["name"] = input.name;
        p["type"] = input.type;
        return p;
    }
    in_type input;
};
static_assert(ApiCP<PlaylistCreate>);

} // namespace api

} // namespace ncm
