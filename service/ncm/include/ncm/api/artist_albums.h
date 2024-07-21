#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct ArtistAlbums {
    model::ArtistId id;
    i64             offset { 0 };
    i64             limit { 25 };
    bool            total { true };
};
} // namespace params

namespace model
{
} // namespace model

namespace api_model
{

struct ArtistAlbums {
    static Result<ArtistAlbums> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<ArtistAlbums>(bs);
    }

    i64                       code;
    bool                      more;
    std::vector<model::Album> hotAlbums;
};
// JSON_DEFINE(ArtistAlbums);

} // namespace api_model

namespace api
{

struct ArtistAlbums {
    using in_type                      = params::ArtistAlbums;
    using out_type                     = api_model::ArtistAlbums;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return fmt::format("/weapi/artist/albums/{}", input.id.as_str()); }
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        convert(p["offset"], input.offset);
        convert(p["limit"], input.limit);
        convert(p["total"], input.total);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<ArtistAlbums>);

} // namespace api

} // namespace ncm
