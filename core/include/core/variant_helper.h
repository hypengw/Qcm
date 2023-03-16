#pragma once

#include <variant>
#include "core/expected_helper.h"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace helper
{

template<typename T, typename... Types>
constexpr std::optional<T> to_optional(const std::variant<Types...>& v) noexcept {
    if (const T* p = std::get_if<T>(&v))
        return *p;
    else
        return std::nullopt;
}

} // namespace helper
