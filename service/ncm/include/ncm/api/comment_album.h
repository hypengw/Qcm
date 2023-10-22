#pragma once

#include "ncm/api.h"
#include "ncm/model.h"

namespace ncm
{

namespace params
{

struct CommentAlbum {
    std::string id;
    i32         offset { 0 };
    i32         limit { 20 };
    model::Time before;
};

} // namespace params

namespace api_model
{

struct CommentAlbum {
    static Result<CommentAlbum> parse(std::span<const byte> bs, const params::CommentAlbum&) {
        return api_model::parse<CommentAlbum>(bs);
    }
    std::vector<model::Comment>                topComments;
    std::optional<std::vector<model::Comment>> hotComments;
    std::vector<model::Comment>                comments;
    i64                                        total;
    std::optional<bool>                        moreHot;
    bool                                       more;
};
JSON_DEFINE(CommentAlbum);
} // namespace api_model

namespace api
{

struct CommentAlbum {
    using in_type                      = params::CommentAlbum;
    using out_type                     = api_model::CommentAlbum;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string path() const {
        return fmt::format("/weapi/v1/resource/comments/R_AL_3_{}", input.id);
    };
    UrlParams query() const { return {}; }
    Params    body() const {
        Params p;
        p["rid"] = input.id;
        convert(p["limit"], input.limit);
        convert(p["offset"], input.offset);
        convert(p["beforeTime"], input.before.milliseconds);
        return p;
    }

    in_type input;
};
static_assert(ApiCP<CommentAlbum>);

} // namespace api

} // namespace ncm
