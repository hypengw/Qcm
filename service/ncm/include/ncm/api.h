#pragma once

#include <concepts>
#include <string_view>

#include "ncm/type.h"
#include "core/str_helper.h"

namespace ncm
{
namespace api_model
{

struct ApiError {
    std::optional<i64>         code;
    std::optional<std::string> message;
    std::optional<std::string> errMsg;
};
JSON_DEFINE(ApiError);
} // namespace api_model
} // namespace ncm

template<>
struct fmt::formatter<ncm::api_model::ApiError> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const ncm::api_model::ApiError& a, FormatContext& ctx) const {
        auto out = fmt::format(
            "code({}) {}", a.code.value_or(-1), a.message.value_or(a.errMsg.value_or("")));
        return fmt::formatter<std::string>::format(out, ctx);
    }
};

namespace ncm
{
namespace api
{

enum class CryptoType
{
    WEAPI,
    EAPI,
    NONE,
};

template<typename T, typename Tin>
concept ApiOutCp = requires(T t, std::span<const byte> bs, Tin tin) {
    { T::parse(bs, tin) } -> std::same_as<Result<T>>;
};

template<typename T>
concept ApiCP = requires(T t) {
    { T::oper } -> std::convertible_to<Operation>;
    { T::crypto } -> std::convertible_to<CryptoType>;
    { t.path() } -> std::convertible_to<std::string_view>;
    { t.query() } -> std::convertible_to<UrlParams>;
    { t.body() } -> ycore::convertible_to_any<Params, BodyReader>;
    { t.input } -> std::convertible_to<typename T::in_type>;
    requires ApiOutCp<typename T::out_type, typename T::in_type>;
};
template<typename T>
concept ApiCP_Base = requires(T t) {
    { T::base } -> std::convertible_to<std::string_view>;
};
template<typename T>
concept ApiCP_BaseFunc = requires(T t) {
    { t.base() } -> std::convertible_to<std::string_view>;
};
template<typename T>
concept ApiCP_Reader = requires(T t) {
    { t.body() } -> std::convertible_to<BodyReader>;
};

template<typename T>
concept ApiCP_Header = requires(T t) {
    { t.header() } -> std::convertible_to<request::Header>;
};

auto concat_query(std::string_view url, std::string_view query) -> std::string;

auto format_api(std::string_view path, const UrlParams& query, const Params& body) -> std::string;
auto format_api(std::string_view path, const UrlParams& query, const BodyReader&) -> std::string;

auto device_id_from_uuid(std::string_view uuid) -> std::string;
} // namespace api

namespace api_model
{

/*
-460 	Cheating
-2 	    无权限访问
200 	ok
201 	已经取消关注
250 	风险提示 // may need os cookie
301 	需要登录
302     system error!
400 	请求(参数异常、无效请求、业务相关)
403 	请求参数异常(无效)
404 	请求的接口不存在
501 	账号不存在
502 	//
503 	验证码错误
405 	发送验证码间隔过短
505 	更新昵称被占用
509 	密码错误超过限制
512 	未付费歌曲无法收藏
 */

template<typename T>
Result<T> parse(std::span<const byte> bs) {
    return json::parse(convert_from<std::string>(bs))
        .map_error([](auto err) {
            return Error::push(err);
        })
        .and_then([](auto j) -> Result<T> {
            if (auto err_ = json::get<ApiError>(*j, {}); err_) {
                auto& err = err_.value();
                if ((err.code && err.code.value() != 200) || err.errMsg)
                    return nstd::unexpected(Error::push(err));
            }

            return json::get<T>(*j, {}).map_error([](auto err) {
                return Error::push(err);
            });
        });
}

template<typename T>
Result<T> parse_no_apierr(std::span<const byte> bs) {
    return json::parse(convert_from<std::string>(bs))
        .and_then([](auto j) {
            return json::get<T>(*j, {});
        })
        .map_error([](auto err) {
            return Error::push(err);
        });
}

} // namespace api_model
} // namespace ncm
