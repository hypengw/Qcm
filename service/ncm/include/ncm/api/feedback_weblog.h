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

    Action                                        act { Action::End };
    std::variant<model::SongId, model::ProgramId> id;

    std::string content;
    std::string mainsite { "1" };

    // End
    // played duration, second
    i64         time;
    i64         wifi { 0 };
    i64         download { 0 };
    std::string alg;
    // interrupt, ui
    std::string end { "playend" };

    // std::string source;
    std::variant<std::monostate, model::PlaylistId, model::AlbumId, model::SpecialId,
                 model::DjradioId>
        sourceId { std::monostate {} };
};
} // namespace params
} // namespace ncm

DEFINE_CONVERT(std::string, ncm::params::FeedbackWeblog::Action) {
    out = in == in_type::Start ? "startplay"sv : "play"sv;
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