#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct ArtistSub {
    model::ArtistId id;
    bool            sub { true };
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct ArtistSub {};
} // namespace model

namespace api_model
{

struct ArtistSub {
    static Result<ArtistSub> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<ArtistSub>(bs);
    }
    i64 code;
};
JSON_DEFINE(ArtistSub);

} // namespace api_model

namespace api
{

struct ArtistSub {
    using in_type                      = params::ArtistSub;
    using out_type                     = api_model::ArtistSub;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const {
        return fmt::format("/weapi/artist/{}", input.sub ? "sub" : "unsub");
    }
    UrlParams query() const { return {}; }
    Params    body() const {
        Params p;
        p["artistId"]  = input.id.as_str();
        p["artistIds"] = fmt::format("[{}]", input.id.as_str());
        return p;
    }
    in_type input;
};
static_assert(ApiCP<ArtistSub>);

} // namespace api

} // namespace ncm
