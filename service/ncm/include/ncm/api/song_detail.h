#pragma once

#include <ranges>
#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{
struct SongDetail {
    std::vector<model::SongId> ids;
};
} // namespace params

namespace api_model
{

struct SongDetail {
    static Result<SongDetail> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<SongDetail>(bs);
    }

    i64 code;
};
JSON_DEFINE(SongDetail);

} // namespace api_model

namespace api
{

struct SongDetail {
    using in_type                      = params::SongDetail;
    using out_type                     = api_model::SongDetail;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/api/v3/song/detail"s; };
    UrlParams   query() const { return {}; }
    Params      body() const {
        auto   view = std::views::transform(input.ids, [](auto& el) -> std::string {
            return fmt::format("{{\"id\": {}}}", el.as_str());
        });
        Params p;
        p["c"] = fmt::format("[{}]", fmt::join(view, ","));
        return {

        };
    }

    in_type input;
};
static_assert(ApiCP<SongDetail>);

} // namespace api

} // namespace ncm
