#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{

struct ArtistSongs {
    model::ArtistId id;
    i32             offset { 0 };
    i32             limit { 50 };
    bool            total { true };

    enum class Order
    {
        Hot,
        Time,
    };

    Order order { Order::Hot };
};

} // namespace params

namespace api_model
{

struct ArtistSongs {
    static Result<ArtistSongs> parse(std::span<const byte> bs, const params::ArtistSongs&) {
        return api_model::parse<ArtistSongs>(bs);
    }

    i64                      code;
    std::vector<model::Song> songs;
    bool                     more;
    i64                      total;
};
JSON_DEFINE(ArtistSongs);

} // namespace api_model

namespace api
{

struct ArtistSongs {
    using in_type                      = params::ArtistSongs;
    using out_type                     = api_model::ArtistSongs;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return fmt::format("/v1/artist/songs", input.id.as_str()); };
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["id"]            = input.id.as_str();
        p["private_cloud"] = "true";
        p["work_type"]     = "1";
        p["order"]         = input.order == params::ArtistSongs::Order::Hot ? "hot" : "time";
        convert(p["offset"], input.offset);
        convert(p["limit"], input.limit);
        convert(p["total"], input.total);
        return p;
    }

    in_type input;
};
static_assert(ApiCP<ArtistSongs>);

} // namespace api

} // namespace ncm
