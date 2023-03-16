#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct PlaylistSubscribe {
    std::string id;
    bool        sub { true };
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct PlaylistSubscribe {};
} // namespace model

namespace api_model
{

struct PlaylistSubscribe {
    static Result<PlaylistSubscribe> parse(std::span<const byte> bs) {
        return api_model::parse<PlaylistSubscribe>(bs);
    }
    // 200
    i64         code;
};
JSON_DEFINE(PlaylistSubscribe);

} // namespace api_model

namespace api
{

struct PlaylistSubscribe {
    using in_type                      = params::PlaylistSubscribe;
    using out_type                     = api_model::PlaylistSubscribe;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::EAPI;

    std::string path() const { return fmt::format("/eapi/playlist/{}", input.sub ? "subscribe" : "unsubscribe"); }
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["id"] = input.id;
        return p;
    }
    in_type input;
};
static_assert(ApiCP<PlaylistSubscribe>);

} // namespace api

}
