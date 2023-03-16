#pragma once

#include <fmt/core.h>
#include <fmt/format.h>

namespace fmt
{

template<typename T>
concept formattable = fmt::is_formattable<T>::value;

}

namespace helper
{
template<typename T>
std::string format_or(const T& e, std::string o) {
    if constexpr (fmt::formattable<T>) {
        return fmt::format("{}", e);
    } else {
        return o;
    }
}

} // namespace helper
