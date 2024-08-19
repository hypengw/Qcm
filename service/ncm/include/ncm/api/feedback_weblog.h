#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct FeedbackWeblog {
    enum class Action
    {
        Start,
        End,
    };

    enum class FileType
    {
        Download  = 0,
        Local     = 1,
        Cache     = 2,
        CloudDesk = 3,
        Online    = 4
    };

    Action                                        act { Action::End };
    FileType                                      file { FileType::Cache };
    std::variant<model::SongId, model::ProgramId> id;
    // 推荐算法字段
    std::string alg;
    std::string end { "playend" };
    // played duration, second
    double time;
    // mill
    double start_log_time;

    // optional
    // 最近常听必传
    std::variant<std::monostate, model::PlaylistId, model::AlbumId, model::SpecialId,
                 model::DjradioId>
        sourceId { std::monostate {} };
};
} // namespace params
} // namespace ncm

DEFINE_CONVERT(std::string, ncm::params::FeedbackWeblog::Action) {
    out = in == in_type::Start ? "startplay"sv : "play"sv;
}
DEFINE_CONVERT(std::string, ncm::params::FeedbackWeblog::FileType) {
    out = std::to_string((i32)in);
}

namespace ncm
{
namespace model
{
struct FeedbackWeblog {};
} // namespace model

namespace api_model
{

struct FeedbackWeblog {
    static Result<FeedbackWeblog> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<FeedbackWeblog>(bs);
    }
    i64         code;
    std::string data;
};
JSON_DEFINE(FeedbackWeblog);

} // namespace api_model

namespace api
{

struct FeedbackWeblog {
    using in_type                      = params::FeedbackWeblog;
    using out_type                     = api_model::FeedbackWeblog;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return "/feedback/weblog"; }
    UrlParams   query() const { return {}; }
    Params      body() const;

    in_type input;
};
static_assert(ApiCP<FeedbackWeblog>);

} // namespace api

} // namespace ncm