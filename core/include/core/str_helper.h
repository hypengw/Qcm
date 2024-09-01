#pragma once
#include <string>
#include <algorithm>
#include <cctype>
#include <array>
#include <concepts>
#include <span>

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

template<typename T>
    requires convertable<std::string, T> && ycore::extra_cvt<std::string, T> &&
             (! requires() { Convert<std::string, T>::AsFormat == false; })
struct fmt::formatter<T> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const T& t, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(convert_from<std::string>(t), ctx);
    }
};

template<usize N>
struct Convert<std::string, char[N]> {
    static void from(std::string& out, const char fmt[N]) {
        out.resize(N);
        std::copy_n(fmt, N, out.begin());
    }
};

template<typename T>
    requires std::integral<T> || std::floating_point<T>
struct Convert<std::string, T> {
    constexpr static bool AsFormat { false };
    static void           from(std::string& out, T in) { out = std::to_string(in); }
};

template<helper::ByteRangeCP Bytes>
struct Convert<std::string, Bytes> {
    static void from(std::string& out, const Bytes& in) {
        out.resize(in.size());
        auto view = std::ranges::transform_view(in, std::to_integer<char>);
        std::copy(view.begin(), view.end(), out.begin());
    }
};

template<typename T>
    requires convertable<std::string, T>
struct Convert<std::string, rc<T>> {
    static void from(std::string& out, const rc<T>& in) { convert(out, *in); }
};

template<typename T>
    requires convertable<std::string, T>
struct Convert<std::string, up<T>> {
    static void from(std::string& out, const up<T>& in) { convert(out, *in); }
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
