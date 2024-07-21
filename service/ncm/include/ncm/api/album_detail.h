#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct AlbumDetail {
    model::AlbumId id;
};
} // namespace params

namespace api_model
{

struct AlbumDetail {
    static Result<AlbumDetail> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<AlbumDetail>(bs);
    }

    i64                      code;
    model::Album             album;
    std::vector<model::Song> songs;
};
JSON_DEFINE(AlbumDetail);

} // namespace api_model

namespace api
{

struct AlbumDetail {
    using in_type                      = params::AlbumDetail;
    using out_type                     = api_model::AlbumDetail;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return fmt::format("/weapi/v1/album/{}", input.id.as_str()); };
    UrlParams   query() const { return {}; }
    Params      body() const { return {}; }

    in_type input;
};
static_assert(ApiCP<AlbumDetail>);

} // namespace api

} // namespace ncm
