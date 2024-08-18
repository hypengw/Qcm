#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/strv_helper.h"

namespace ncm
{

namespace params
{

struct UploadAddr {
    std::string bucket;
};

} // namespace params

namespace api_model
{

struct UploadAddr {
    static Result<UploadAddr> parse(std::span<const byte> bs, const params::UploadAddr&) {
        return api_model::parse<UploadAddr>(bs);
    }

    // httpDNS的IP访问地址，用于避免后续查询的DNS解析时间、以及域名劫持
    std::string lbs;
    // 上传节点列表（前面的优先级高）
    std::vector<std::string> upload;
};
JSON_DEFINE(UploadAddr);

} // namespace api_model

namespace api
{

struct UploadAddr {
    using in_type                      = params::UploadAddr;
    using out_type                     = api_model::UploadAddr;
    constexpr static Operation  oper   = Operation::GetOperation;
    constexpr static CryptoType crypto = CryptoType::NONE;
    constexpr static auto       base   = "https://wanproxy.127.net"sv;

    std::string path() const { return "/lbs"; };
    UrlParams   query() const {
        UrlParams q;
        q.set_param("version", "1.0");
        q.set_param("bucketname", input.bucket);
        return q;
    }
    Params body() const { return {}; }

    in_type input;
};
static_assert(ApiCP<UploadAddr>);

} // namespace api

} // namespace ncm
