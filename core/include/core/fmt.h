#pragma once


#ifdef __clangd__
#include "core/clangd.h"
#else
import qcm.core;
#endif

#include <format>
#include <span>
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

template<typename It, typename Sentinel, typename Char = char>
struct join_view {
    It                           begin;
    Sentinel                     end;
    std::basic_string_view<Char> sep;

    join_view(It b, Sentinel e, std::basic_string_view<Char> s)
        : begin(std::move(b)), end(e), sep(s) {}
};

template<typename It, typename Sentinel>
auto join(It begin, Sentinel end, std::string_view sep) -> join_view<It, Sentinel> {
    return { std::move(begin), end, sep };
}

template<typename Range>
    requires(! ycore::tuple_like<Range>)
auto join(Range&& r, std::string_view sep)
    -> join_view<decltype(std::begin(r)), decltype(std::end(r))> {
    return { std::begin(r), std::end(r), sep };
}

} // namespace helper
template<typename It, typename Sentinel, typename Char>
struct std::formatter<helper::join_view<It, Sentinel, Char>, Char> {
private:
    using value_type = typename std::iterator_traits<It>::value_type;
    std::formatter<remove_cvref_t<value_type>, Char> value_formatter_;

    using view = std::conditional_t<std::is_copy_constructible<It>::value,
                               const helper::join_view<It, Sentinel, Char>, helper::join_view<It, Sentinel, Char>>;

public:
    using nonlocking = void;

    constexpr auto parse(std::basic_format_parse_context<Char>& ctx) -> const Char* {
        return value_formatter_.parse(ctx);
    }

    template<typename FormatContext>
    auto format(view& value, FormatContext& ctx) const -> decltype(ctx.out()) {
        using iter = conditional_t<std::is_copy_constructible<view>::value, It, It&>;
        iter it    = value.begin;
        auto out   = ctx.out();
        if (it == value.end) return out;
        out = value_formatter_.format(*it, ctx);
        ++it;
        while (it != value.end) {
            out.append(value.sep.begin(), value.sep.end());
            // out = fmt::detail::copy<Char>(value.sep.begin(), value.sep.end(), out);
            ctx.advance_to(out);
            out = value_formatter_.format(*it, ctx);
            ++it;
        }
        return out;
    }
};

template<>
struct fmt::range_formatter<std::byte, char> : fmt::range_formatter<char, char> {
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
        std::span<const char> view { static_cast<char*>(range.data()), std::size(range) };
        return base_t::format(view, ctx);
    }
};
