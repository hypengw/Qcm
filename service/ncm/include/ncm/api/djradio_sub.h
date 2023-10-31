#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct DjradioSub {
    std::string id;
    bool        sub { true };
};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct DjradioSub {};
} // namespace model

namespace api_model
{

struct DjradioSub {
    static Result<DjradioSub> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<DjradioSub>(bs);
    }

    i64 code;
};
JSON_DEFINE(DjradioSub);

} // namespace api_model

namespace api
{

struct DjradioSub {
    using in_type                      = params::DjradioSub;
    using out_type                     = api_model::DjradioSub;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const { return fmt::format("/weapi/djradio/{}", input.sub ? "sub" : "unsub"); }
    UrlParams   query() const { return {}; }
    Params      body() const {
        Params p;
        p["id"] = input.id;
        return p;
    }
    in_type input;
};
static_assert(ApiCP<DjradioSub>);

} // namespace api

} // namespace ncm
