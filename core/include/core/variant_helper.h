#pragma once

#include <variant>
#include <utility>
#include "core/expected_helper.h"

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace helper
{

namespace detail
{

template<typename T, std::size_t I, std::size_t... Is>
T make_variant(const std::size_t i, std::index_sequence<I, Is...>) {
    if constexpr (sizeof...(Is) > 0) {
        return i == I ? T(std::in_place_index<I>)
                      : make_variant<T>(i, std::index_sequence<Is...> {});
    } else {
        return T(std::in_place_index<I>);
    }
}

} // namespace detail

template<typename... Ts>
std::variant<Ts...> make_variant(const std::size_t i) {
    return detail::make_variant<std::variant<Ts...>>(i, std::index_sequence_for<Ts...> {});
}

template<typename T, typename... Types>
constexpr std::optional<T> to_optional(const std::variant<Types...>& v) noexcept {
    if (const T* p = std::get_if<T>(&v))
        return *p;
    else
        return std::nullopt;
}

} // namespace helper
