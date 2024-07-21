#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct FeedbackWeblog {
    model::SongId id;
    std::string   sourceId;
    model::Time   time;
};
} // namespace params
} // namespace ncm

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
    i64 code;
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

    std::string path() const { return "/weapi/feedback/weblog"; }
    UrlParams   query() const { return {}; }
    Params      body() const;

    in_type input;
};
static_assert(ApiCP<FeedbackWeblog>);

} // namespace api

} // namespace ncm