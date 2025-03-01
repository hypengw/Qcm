#pragma once

#include <format>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

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
