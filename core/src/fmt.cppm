module;
#include <format>
#include <span>
export module qcm.core:fmt;
export import :basic;

namespace helper
{
export template<typename T>
std::string format_or(const T& e, std::string o) {
    if constexpr (std::formattable<T, char>) {
        return std::format("{}", e);
    } else {
        return o;
    }
}

export template<typename It, typename Sentinel, typename Char = char>
struct join_view {
    It                           begin;
    Sentinel                     end;
    std::basic_string_view<Char> sep;

    join_view(It b, Sentinel e, std::basic_string_view<Char> s)
        : begin(std::move(b)), end(e), sep(s) {}
};

export template<typename It, typename Sentinel>
auto join(It begin, Sentinel end, std::string_view sep) -> join_view<It, Sentinel> {
    return { std::move(begin), end, sep };
}

export template<typename Range>
    requires(! ycore::tuple_like<Range>)
auto join(Range&& r, std::string_view sep)
    -> join_view<decltype(std::begin(r)), decltype(std::end(r))> {
    return { std::begin(r), std::end(r), sep };
}

} // namespace helper
export template<typename It, typename Sentinel, typename Char>
struct rstd::Impl<rstd::fmt::Display, helper::join_view<It, Sentinel, Char>>
    : rstd::ImplBase<helper::join_view<It, Sentinel, Char>> {
    auto fmt(rstd::fmt::Formatter& f) const -> bool {
        auto& value = this->self();
        auto  it    = value.begin;
        if (it == value.end) return true;

        if (! rstd::as<rstd::fmt::Display>(*it).fmt(f)) return false;
        ++it;
        while (it != value.end) {
            if (! f.write_raw((const u8*)value.sep.data(), value.sep.size())) return false;
            if (! rstd::as<rstd::fmt::Display>(*it).fmt(f)) return false;
            ++it;
        }
        return true;
    }
};
/*
template<>
struct std::range_formatter<std::byte, char> : std::range_formatter<char, char> {
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
*/