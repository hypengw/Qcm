#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct ArtistSublist {
    i64  offset { 0 };
    i64  limit { 25 };
    bool total { true };
};
} // namespace params

namespace model
{
struct ArtistSublistItem {
    std::string                info;
    i64                        id;
    std::string                name;
    std::optional<std::string> trans;
    std::vector<std::string>   alias;
    i64                        albumSize;
    i64                        mvSize;
    // "picId": 109951165608475961,
    std::string picUrl;
    std::string img1v1Url;
};
} // namespace model

namespace api_model
{

struct ArtistSublist {
    static Result<ArtistSublist> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<ArtistSublist>(bs);
    }

    i64 code;
    std::optional<i64>                    count;
    bool                                  hasMore;
    std::vector<model::ArtistSublistItem> data;
};
// JSON_DEFINE(ArtistSublist);

} // namespace api_model

namespace api
{

struct ArtistSublist {
    using in_type                      = params::ArtistSublist;
    using out_type                     = api_model::ArtistSublist;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/artist/sublist"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        convert(p["offset"], input.offset);
        convert(p["limit"], input.limit);
        convert(p["total"], input.total);
        return p;
    }
    in_type input;
};
static_assert(ApiCP<ArtistSublist>);

} // namespace api

} // namespace ncm
