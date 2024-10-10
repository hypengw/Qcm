#pragma once

#include <string_view>
#include <algorithm>
#include <vector>

#include <core/core.h>

using namespace std::literals::string_view_literals;

namespace helper
{

constexpr std::string_view trims_left(std::string_view in) noexcept {
    auto iter = std::find_if(in.begin(), in.end(), [](char c) {
        return ! std::isspace(c);
    });
    return { iter != in.end() ? iter : in.begin(), in.end() };
}

constexpr std::string_view trims_right(std::string_view in) noexcept {
    auto iter = std::find_if(in.rbegin(), in.rend(), [](char c) {
        return ! std::isspace(c);
    });
    return { in.begin(), iter != in.rend() ? iter.base() : in.end() };
}

constexpr std::string_view trims(std::string_view in) noexcept {
    return trims_left(trims_right(in));
}

constexpr auto split(std::string_view            in,
                     std::span<std::string_view> seps) -> std::vector<std::string_view> {
    std::vector<std::string_view> out;
    auto                          last_it = in.begin();
    auto                          it      = in.begin();
    auto                          end     = in.end();
    while (it < end) {
        auto sub = std::string_view(it, end);
        for (auto sep : seps) {
            if (sub.starts_with(sep)) {
                out.emplace_back(last_it, it);
                it += sep.size();
                last_it = it;
                continue;
            }
        }
        it++;
    }
    if (it != last_it) out.emplace_back(last_it, it);
    return out;
}

constexpr auto case_insensitive_compare(std::string_view a, std::string_view b) noexcept {
    return std::lexicographical_compare_three_way(
        a.begin(), a.end(), b.begin(), b.end(), [](unsigned char a, unsigned char b) {
            const auto la = std::tolower(a);
            const auto lb = std::tolower(b);
            return (la < lb)   ? std::weak_ordering::less
                   : (la > lb) ? std::weak_ordering::greater
                               : std::weak_ordering::equivalent;
        });
}

constexpr bool starts_with_i(std::string_view str, std::string_view start) noexcept {
    return str.size() >= start.size() &&
           case_insensitive_compare({ str.begin(), str.begin() + start.size() }, start) == 0;
}

inline auto get_env_var(std::string_view var_name) -> std::optional<std::string_view> {
    if (const char* value = std::getenv(var_name.data())) {
        return std::string_view(value);
    }
    return std::nullopt;
}

} // namespace helper

template<typename T>
    requires std::ranges::sized_range<T> &&
             (std::convertible_to<std::ranges::range_value_t<T>, char> ||
              std::convertible_to<std::ranges::range_value_t<T>, byte>) &&
             (sizeof(std::ranges::range_value_t<T>) == sizeof(byte))
struct Convert<std::string_view, T> {
    static void from(std::string_view& out, const T& in) {
        out = std::string_view { (const char*)in.data(), in.size() };
    }
};