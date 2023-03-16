#pragma once

#include <tl/expected.hpp>
#include <optional>

#include "core/core.h"
#include "core/log.h"
#include "core/fmt.h"

#define UNWRAP(_RES_, _R_)                                                                  \
    do {                                                                                    \
        auto _exp = helper::to_expected(_R_);                                               \
        _assert_msg_(                                                                       \
            _exp.has_value(), "unwrap {}", helper::format_or(std::move(_exp).error(), "")); \
        _RES_ = std::move(_exp).value();                                                    \
    } while (false)

#define ERR_RET(_RES_, _R_)                                           \
    do {                                                              \
        if (auto _exp = helper::to_expected(_R_); ! _exp.has_value()) \
            return nstd::unexpected(std::move(_exp).error()));        \
        else                                                          \
            _RES_ = std::move(_exp).value()                           \
    } while (false)

#define ERR_RET_CO(_RES_, _R_)                                        \
    do {                                                              \
        if (auto _exp = helper::to_expected(_R_); ! _exp.has_value()) \
            co_return nstd::unexpected(std::move(_exp.error()));      \
        else                                                          \
            _RES_ = std::move(_exp).value();                          \
    } while (false)

namespace nstd
{
template<typename T, typename F>
using expected = tl::expected<T, F>;

template<typename T>
auto unexpected(T&& v) {
    return tl::unexpected(std::forward<T>(v));
}

template<typename T>
concept is_expected = core::is_specialization_of<T, nstd::expected>;

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

template<typename T, typename F>
std::optional<T> to_optional(tl::expected<T, F> res) {
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

} // namespace helper
