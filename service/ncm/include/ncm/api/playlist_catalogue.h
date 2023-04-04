#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{

struct PlaylistCatalogue {};
} // namespace params

namespace model
{
struct PlaylistCatalogue {
    std::string name;
    i64         resourceCount;
    i64         imgId;
    i64         type;
    i64         category;
    i64         resourceType;
    bool        hot;
    bool        activity;
    // imgUrl
};
} // namespace model

namespace api_model
{

struct PlaylistCatalogue {
    static Result<PlaylistCatalogue> parse(std::span<const byte> bs) {
        return api_model::parse<PlaylistCatalogue>(bs);
    }

    i64                                          code;
    model::PlaylistCatalogue                     all;
    std::vector<model::PlaylistCatalogue>        sub;
    // std::unordered_map<std::string, std::string> categories;
};
JSON_DEFINE(PlaylistCatalogue);

} // namespace api_model

namespace api
{

struct PlaylistCatalogue {
    using in_type                      = params::PlaylistCatalogue;
    using out_type                     = api_model::PlaylistCatalogue;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/playlist/catalogue"; };
    UrlParams        query() const { return {}; }
    Params           body() const { return {}; }
    in_type          input;
};
static_assert(ApiCP<PlaylistCatalogue>);

} // namespace api

} // namespace ncm
