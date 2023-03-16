#pragma once

#include <concepts>
#include <string_view>

#include "ncm/type.h"
#include "core/str_helper.h"

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

template<typename T>
concept ApiOutCp = requires(T t, std::span<const byte> bs) {
                       { T::parse(bs) } -> std::same_as<Result<T>>;
                   };

template<typename T>
concept ApiCP = requires(T t) {
                    { T::oper } -> std::convertible_to<Operation>;
                    { T::crypto } -> std::convertible_to<CryptoType>;
                    { t.path() } -> std::convertible_to<std::string_view>;
                    { t.query() } -> std::convertible_to<UrlParams>;
                    { t.body() } -> std::convertible_to<Params>;
                    { t.input } -> std::convertible_to<typename T::in_type>;
                    requires ApiOutCp<typename T::out_type>;
                };
template<typename T>
concept ApiCP_Base = requires(T t) {
                         { T::base } -> std::convertible_to<std::string_view>;
                     };

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
    requires api::ApiOutCp<T>
Result<T> parse(std::span<const byte> bs) {
    return json::parse(To<std::string>::from(bs))
        .and_then([](auto j) {
            return json::get<T>(*j, {});
        })
        .map_error([](auto err) {
            return Error::push(err);
        });
}

} // namespace api_model
} // namespace ncm
