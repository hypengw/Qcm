#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct AlbumSub {
    model::AlbumId id;
    bool           sub { true };
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct AlbumSub {};
} // namespace model

namespace api_model
{

struct AlbumSub {
    static Result<AlbumSub> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<AlbumSub>(bs);
    }
    // 200
    i64         code;
    model::Time time;
};
JSON_DEFINE(AlbumSub);

} // namespace api_model

namespace api
{

struct AlbumSub {
    using in_type                      = params::AlbumSub;
    using out_type                     = api_model::AlbumSub;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return fmt::format("/weapi/album/{}", input.sub ? "sub" : "unsub"); }
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["id"] = input.id.as_str();
        return p;
    }
    in_type input;
};
static_assert(ApiCP<AlbumSub>);

} // namespace api

} // namespace ncm
