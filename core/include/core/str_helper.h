#pragma once
#include <string_view>
#include <string>
#include <algorithm>
#include <cctype>
#include <array>
#include <concepts>

#include "core/core.h"
#include "core/fmt.h"
#include "core/vec_helper.h"

namespace helper
{
template<typename T>
concept ByteRangeCP =
    std::ranges::range<T> && std::same_as<std::decay_t<std::ranges::range_value_t<T>>, byte>;
} // namespace helper

template<helper::ByteRangeCP Bytes>
struct fmt::formatter<Bytes> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const Bytes& bs, FormatContext& ctx) const {
        std::string out;
        for (auto& b : bs) {
            out.push_back(std::to_integer<char>(b));
        }
        return fmt::formatter<std::string>::format(out, ctx);
    }
};

template<fmt::formattable T>
struct Convert<std::string, T> {
    static void from(std::string& out, const T& fmt) { out = fmt::format("{}", fmt); }
};

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

template<size_t N>
struct literal_string_bytes {
    std::array<std::byte, N - 1> arr;
    constexpr literal_string_bytes(const char (&in)[N]) {
        std::transform(in, in + N - 1, arr.begin(), [](unsigned char c) {
            return std::byte { c };
        });
    }
};

} // namespace helper

template<helper::literal_string_bytes s>
constexpr auto operator"" _sb() {
    return s.arr;
}
