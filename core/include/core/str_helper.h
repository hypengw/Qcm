#pragma once
#include <string>
#include <algorithm>
#include <cctype>
#include <array>
#include <concepts>

#include "core/core.h"
#include "core/fmt.h"
#include "core/vec_helper.h"
#include "core/strv_helper.h"

using namespace std::literals::string_literals;

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
    requires(ycore::extra_cvt<std::string, T>)
struct Convert<std::string, T> {
    static void from(std::string& out, const T& fmt) { out = fmt::format("{}", fmt); }
};

namespace helper
{

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
