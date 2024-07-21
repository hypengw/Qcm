#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{

struct Artist {
    model::ArtistId id;
};

} // namespace params

namespace api_model
{

struct Artist {
    static Result<Artist> parse(std::span<const byte> bs, const params::Artist&) { 
        return api_model::parse<Artist>(bs); 
    }

    i64                      code;
    model::Artist            artist;
    std::vector<model::Song> hotSongs;
    bool                     more;
};
JSON_DEFINE(Artist);

} // namespace api_model

namespace api
{

struct Artist {
    using in_type                      = params::Artist;
    using out_type                     = api_model::Artist;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return fmt::format("/weapi/v1/artist/{}", input.id.as_str()); };
    UrlParams   query() const { return {}; }
    Params      body() const { return {}; }

    in_type input;
};
static_assert(ApiCP<Artist>);

} // namespace api

} // namespace ncm
