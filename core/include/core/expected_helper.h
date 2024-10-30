#pragma once

#include <tl/expected.hpp>
#include <optional>

#include "core/core.h"
#include "core/log.h"
#include "core/fmt.h"
#include "core/optional_helper.h"

#define UNWRAP(EXP) helper::unwrap(EXP, #EXP)

#define UE(EXP)                                                    \
    (                                                              \
        {                                                          \
            if (! EXP.has_value()) return helper::unexpected(exp); \
        },                                                         \
        EXP.value())

#define ERR_RET(_RES_, _R_)                 \
    do {                                    \
        auto exp = (_R_);                   \
        if (! exp.has_value())              \
            return helper::unexpected(exp); \
        else                                \
            _RES_ = std::move(exp).value(); \
    } while (false)

#define ERR_RET_CO(_RES_, _R_)                 \
    do {                                       \
        auto exp = (_R_);                      \
        if (! exp.has_value())                 \
            co_return helper::unexpected(exp); \
        else                                   \
            _RES_ = std::move(exp).value();    \
    } while (false)

#define UNEXPECTED(_V_) nstd::unexpected(_V_)

namespace nstd
{
template<typename T, typename F>
using expected = tl::expected<T, F>;

template<typename T>
auto unexpected(T&& v) {
    return tl::unexpected(std::forward<T>(v));
}

template<typename T>
concept is_expected = ycore::is_specialization_of_v<T, tl::expected>;

namespace detail_expected
{

template<typename T>
struct inner_type {
    using type = T;
};

template<typename T>
using inner_type_t = typename inner_type<T>::type;

template<typename T, typename F>
struct inner_type<nstd::expected<T, F>> {
    using type = inner_type_t<T>;
};

} // namespace detail_expected

template<typename T>
using expected_inner = detail_expected::inner_type_t<T>;

template<typename T, typename F, typename R = expected_inner<T>>
nstd::expected<R, F> expected_unpack(nstd::expected<T, F>&& expr) {
    if constexpr (nstd::is_expected<T>) {
        if (expr.has_value())
            return expected_unpack(std::move(expr).value());
        else
            return nstd::unexpected(std::move(expr).error());
    } else
        return std::move(expr);
}

} // namespace nstd

namespace helper
{

template<typename T>
concept is_expected = ycore::is_specialization_of_v<std::decay_t<T>, tl::expected>;

template<typename T, typename Fn>
auto map(T&& t, Fn&& fn) -> std::optional<decltype(fn(t.value()))> {
    if (t) return fn(t.value());
    return std::nullopt;
}

template<typename T, typename F>
auto to_optional(tl::expected<T, F> res) -> std::optional<T> {
    if (res.has_value())
        return std::move(res).value();
    else
        return std::nullopt;
}

template<typename T>
nstd::expected<T, std::nullopt_t> to_expected(std::optional<T> opt) {
    if (opt.has_value())
        return std::move(opt).value();
    else
        return nstd::unexpected(std::nullopt);
}

template<typename T, typename E>
auto to_expected(nstd::expected<T, E> res) {
    return res;
}

template<typename T, typename E>
nstd::expected<E, T> expected_switch(nstd::expected<T, E> res) {
    if (res.has_value())
        return nstd::unexpected(std::move(res).value());
    else
        return std::move(res).error();
}

template<typename T>
    requires is_optional<T>
typename std::decay_t<T>::value_type value_or_default(T&& t) {
    if (t.has_value())
        return std::forward<T>(t).value();
    else
        return typename std::decay_t<T>::value_type {};
}

template<typename T>
    requires is_optional<T> || is_expected<T>
auto unwrap(T&& opt, std::string_view expr = {},
            const std::source_location loc = std::source_location::current()) {
    if constexpr (is_expected<T>) {
        _tpl_assert_msg_(true,
                         opt.has_value(),
                         loc,
                         "unwrap{}faild: {}",
                         expr.empty() ? " " : fmt::format(" `{}` ", expr),
                         opt.error());
    } else {
        _tpl_assert_msg_(true,
                         opt.has_value(),
                         loc,
                         "unwrap{}faild",
                         expr.empty() ? " " : fmt::format(" `{}` ", expr));
    }
    return std::forward<T>(opt).value();
}

template<typename T>
    requires is_optional<T>
auto unexpected(T&&) {
    return std::nullopt;
}

template<typename T>
    requires is_expected<T>
auto unexpected(T&& t) {
    return nstd::unexpected(std::forward<T>(t).error());
}

} // namespace helper