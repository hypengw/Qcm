#pragma once
#include <ranges>
#include "core/fmt.h"
import qcm.helper;

namespace helper
{
template<typename T>
concept ByteRangeCP =
    std::ranges::range<T> && std::same_as<std::decay_t<std::ranges::range_value_t<T>>, byte>;
} // namespace helper

template<typename T, typename F>
    requires std::same_as<T, std::string_view> && std::ranges::sized_range<T> &&
             (std::convertible_to<std::ranges::range_value_t<T>, char> ||
              std::convertible_to<std::ranges::range_value_t<T>, byte>) &&
             (sizeof(std::ranges::range_value_t<T>) == sizeof(byte))
struct Convert<T, F> {
    static void from(std::string_view& out, const F& in) {
        out = std::string_view { (const char*)in.data(), (std::size_t)in.size() };
    }
};

template<typename T, helper::ByteRangeCP Bytes>
    requires std::same_as<T, std::string>
struct Convert<T, Bytes> {
    // already defined by fmt
    constexpr static bool AsFormat { false };
    static void           from(std::string& out, const Bytes& in) {
        out.resize(in.size());
        auto view = std::ranges::transform_view(in, std::to_integer<char>);
        std::copy(view.begin(), view.end(), out.begin());
    }
};

template<>
struct fmt::range_formatter<byte, char> : fmt::range_formatter<char, char> {
    using Char   = char;
    using base_t = fmt::range_formatter<char, char>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
        auto it  = ctx.begin();
        char cur = it != ctx.end() ? detail::to_ascii(*it) : '\0';
        auto out = base_t::parse(ctx);
        // clear string bracket
        if (cur == 's') {
            set_brackets({}, {});
        }
        return out;
    }

    template<typename R, typename FormatContext>
    auto format(R&& range, FormatContext& ctx) const -> decltype(ctx.out()) {
        auto view = std::ranges::transform_view(range, [](const byte b) -> char {
            return (char)(b);
        });
        return base_t::format(view, ctx);
    }
};

template<typename T, typename F>
    requires ycore::is_specialization_of_v<T, std::vector> && std::ranges::range<F> &&
             convertable<std::ranges::range_value_t<T>, std::ranges::range_value_t<F>>
struct Convert<T, F> {
    static void from(T& out, const F& f) {
        using to_value_type   = std::ranges::range_value_t<T>;
        using from_value_type = std::ranges::range_value_t<F>;
        out.clear();
        std::transform(std::ranges::begin(f),
                       std::ranges::end(f),
                       std::back_inserter(out),
                       [](const from_value_type& v) {
                           return convert_from<to_value_type>(v);
                       });
    }
};