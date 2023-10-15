#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct SongUrl {
    enum class Level
    {
        Standard = 0,
        Higher,
        Exhigh,
        Lossless,
        Hires
    };
    std::vector<std::string> ids;
    Level                    level { Level::Exhigh };
    std::string              encodeType { "flac" };
};
} // namespace params
} // namespace ncm

template<>
struct fmt::formatter<ncm::params::SongUrl::Level> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(ncm::params::SongUrl::Level l, FormatContext& ctx) const {
        std::string_view out;
        switch (l) {
            using enum ncm::params::SongUrl::Level;
        case Standard: out = "standard"; break;
        case Higher: out = "higher"; break;
        case Lossless: out = "lossless"; break;
        case Hires: out = "hires"; break;
        case Exhigh:
        default: out = "exhigh"; break;
        }
        return fmt::formatter<std::string_view>::format(out, ctx);
    }
};

namespace ncm
{
namespace model
{
struct SongUrl {
    i64         id;
    std::string url;
    i64         br;
    i64         size;
    std::string md5;
    std::string type;
    i64         fee;
    std::string level;
    std::string encodeType;
    i64         time;
};
} // namespace model

namespace api_model
{

struct SongUrl {
    static Result<SongUrl> parse(std::span<const byte> bs, const auto&) { return api_model::parse<SongUrl>(bs); }
    std::vector<model::SongUrl> data;
    i64                         code;
};
JSON_DEFINE(SongUrl);

} // namespace api_model

namespace api
{

struct SongUrl {
    using in_type                            = params::SongUrl;
    using out_type                           = api_model::SongUrl;
    constexpr static Operation        oper   = Operation::PostOperation;
    constexpr static CryptoType       crypto = CryptoType::EAPI;
    constexpr static std::string_view base   = "https://interface.music.163.com";

    std::string_view path() const { return "/eapi/song/enhance/player/url/v1"; }
    UrlParams        query() const { return {}; }
    Params           body() const {
        Params p;
        p["ids"]        = fmt::format("[{}]", fmt::join(input.ids, ","));
        p["level"]      = fmt::format("{}", input.level);
        p["encodeType"] = input.encodeType;
        return p;
    }
    in_type input;
};
static_assert(ApiCP<SongUrl>);

} // namespace api

} // namespace ncm
