#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct SongLyric {
    model::SongId id;
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct SongLyricItem {
    i64         version;
    std::string lyric;
};
} // namespace model

namespace api_model
{

struct SongLyric {
    static Result<SongLyric> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<SongLyric>(bs);
    }
    // 200
    i64 code;
    // sgc, sfy, qfy
    model::SongLyricItem                lrc;
    std::optional<model::SongLyricItem> klyric;
    std::optional<model::SongLyricItem> tlyric;
    std::optional<model::SongLyricItem> romalrc;
};
JSON_DEFINE(SongLyric);

} // namespace api_model

namespace api
{

struct SongLyric {
    using in_type                      = params::SongLyric;
    using out_type                     = api_model::SongLyric;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/song/lyric"; }
    UrlParams        query() const {
        UrlParams p;
        p.set_param("_nmclfl", "1");
        return p;
    }
    Params body() const {
        Params p;
        p["id"] = input.id.as_str();
        p["tv"] = "-1";
        p["lv"] = "-1";
        p["rv"] = "-1";
        p["kv"] = "-1";
        return p;
    }
    in_type input;
};
static_assert(ApiCP<SongLyric>);

} // namespace api

} // namespace ncm
