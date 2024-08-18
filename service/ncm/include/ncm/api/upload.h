#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/strv_helper.h"

namespace ncm
{

namespace params
{

struct Upload {
    std::string upload_host;
    std::string bucket;
    std::string object;
    i64         offset { 0 };
    bool        complete { true };

    std::string nos_token;
    std::string content_type;
    std::string content_md5;
    usize       size;
    BodyReader  reader;
};

} // namespace params

namespace api_model
{

struct Upload {
    static Result<Upload> parse(std::span<const byte> bs, const params::Upload&) {
        return api_model::parse<Upload>(bs);
    }

    // uuid字符串，服务器端生成的唯一UUID
    std::string requestId;
    //	下一个上传片在上传块中的偏移。注意：偏移从0开始，比如：用户上传0-128字节后，服务器返回的offset为128，下一次上传offset值应置为128
    i64 offset;

    // 上传上下文
    // std::string context;

    //	上传回调信息
    //  std::string callbackRetMsg;
};
JSON_DEFINE(Upload);

} // namespace api_model

namespace api
{

struct Upload {
    using in_type                      = params::Upload;
    using out_type                     = api_model::Upload;
    constexpr static Operation  oper   = Operation::GetOperation;
    constexpr static CryptoType crypto = CryptoType::NONE;

    auto base() const -> std::string { return input.upload_host; }
    auto path() const -> std::string { return std::format("/{}/{}", input.bucket, input.object); };
    auto query() const -> UrlParams {
        UrlParams q;
        q.set_param("version", "1.0");
        q.set_param("offset", convert_from<std::string>(input.offset));
        q.set_param("complete", convert_from<std::string>(input.complete));
        return q;
    }
    auto header() const -> request::Header {
        request::Header h;
        h.insert({ "x-nos-token", input.nos_token });
        h.insert({ "Content-Type", input.content_type });
        h.insert({ "Content-MD5", input.content_md5 });
        h.insert({ "Content-Length", convert_from<std::string>(input.size) });
        return h;
    }
    auto body() const -> BodyReader { return input.reader; };

    in_type input;
};
static_assert(ApiCP<Upload>);

} // namespace api

} // namespace ncm
