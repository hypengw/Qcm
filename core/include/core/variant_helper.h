#pragma once

#include <variant>
#include <utility>
#include "core/expected_helper.h"
#include "core/type_list.h"

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
auto make_variant(const std::size_t i, std::index_sequence<I, Is...>) -> T {
    if constexpr (sizeof...(Is) > 0) {
        return i == I ? T(std::in_place_index<I>)
                      : make_variant<T>(i, std::index_sequence<Is...> {});
    } else {
        return T(std::in_place_index<I>);
    }
}

} // namespace detail

template<typename... Ts>
auto make_variant(const std::size_t i) -> std::variant<Ts...> {
    return detail::make_variant<std::variant<Ts...>>(i, std::index_sequence_for<Ts...> {});
}

template<typename T, typename... Types>
constexpr auto to_optional(const std::variant<Types...>& v) noexcept -> std::optional<T> {
    if (const T* p = std::get_if<T>(&v))
        return *p;
    else
        return std::nullopt;
}

template<typename... ToTs, typename... InTs>
bool variant_convert(std::variant<ToTs...>& out, const std::variant<InTs...>& in) {
    return std::visit(
        overloaded { [&out]<typename T>(const T& in) -> bool
                         requires(ycore::type_list<ToTs...>::template contains<T>())
                                 {
                                     out = in;
                                     return true;
                                 },
                                 []<typename T>(const T&) -> bool
                                     requires(! ycore::type_list<ToTs...>::template contains<T>())
                     {
                         return false;
                     } },
                     in);
}

} // namespace helper
