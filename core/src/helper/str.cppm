module;
#include <string>
#include <string_view>
#include <algorithm>
#include <cctype>
#include <array>
#include <concepts>

#include "core/fmt.h"

export module qcm.helper:str;
export import qcm.core;
import :container;

export using namespace std::literals::string_literals;
export using namespace std::literals::string_view_literals;

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
template<typename T>
concept ByteRangeCP = ycore::range<T> && std::same_as<std::decay_t<ycore::range_value_t<T>>, byte>;

} // namespace helper

export template<helper::literal_string_bytes s>
constexpr auto operator""_sb() {
    return s.arr;
}

namespace helper
{

export constexpr std::string_view trims_left(std::string_view in) noexcept {
    auto iter = std::find_if(in.begin(), in.end(), [](char c) {
        return ! std::isspace(c);
    });
    return { iter != in.end() ? iter : in.begin(), in.end() };
}

export constexpr std::string_view trims_right(std::string_view in) noexcept {
    auto iter = std::find_if(in.rbegin(), in.rend(), [](char c) {
        return ! std::isspace(c);
    });
    return { in.begin(), iter != in.rend() ? iter.base() : in.end() };
}

export constexpr std::string_view trims(std::string_view in) noexcept {
    return trims_left(trims_right(in));
}

export constexpr auto split(std::string_view in, std::span<std::string_view> seps)
    -> std::vector<std::string_view> {
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

export constexpr auto case_insensitive_compare(std::string_view a, std::string_view b) noexcept {
    return std::lexicographical_compare_three_way(
        a.begin(), a.end(), b.begin(), b.end(), [](unsigned char a, unsigned char b) {
            const auto la = std::tolower(a);
            const auto lb = std::tolower(b);
            return (la < lb)   ? std::weak_ordering::less
                   : (la > lb) ? std::weak_ordering::greater
                               : std::weak_ordering::equivalent;
        });
}

export constexpr bool starts_with_i(std::string_view str, std::string_view start) noexcept {
    return str.size() >= start.size() &&
           case_insensitive_compare({ str.begin(), str.begin() + start.size() }, start) == 0;
}

export auto get_env_var(std::string_view var_name) -> std::optional<std::string_view> {
    if (const char* value = std::getenv(var_name.data())) {
        return std::string_view(value);
    }
    return std::nullopt;
}

export auto to_upper(std::string_view in) -> std::string {
    std::string out(in);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
    return out;
}

export auto to_lower(std::string_view in) -> std::string {
    std::string out(in);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return out;
}
export template<typename T>
concept convert_formatable = (! requires() {
    { Convert<std::string, T>::AsFormat } -> std::convertible_to<bool>;
} || Convert<std::string, T>::AsFormat == true);

} // namespace helper

export template<typename T>
    requires convertable<std::string, T> && ycore::extra_cvt<std::string, T> &&
             helper::convert_formatable<T>
struct fmt::formatter<T> : fmt::formatter<std::string> {
    auto format(const T& t, format_context& ctx) const -> format_context::iterator {
        return fmt::formatter<std::string>::format(convert_from<std::string>(t), ctx);
    }
};

export template<typename T, typename F>
    requires std::same_as<T, std::string> && std::is_array_v<F> &&
             std::same_as<typename helper::array_traits<F>::type, char>
struct Convert<T, F> {
    static constexpr auto N = helper::array_traits<F>::size;
    static void           from(std::string& out, const char fmt[N]) {
        out.resize(N);
        std::copy_n(fmt, N, out.begin());
    }
};

export template<typename T, typename F>
    requires std::same_as<T, std::string> && (std::integral<F> || std::floating_point<F>)
struct Convert<T, F> {
    constexpr static bool AsFormat { false };
    static void           from(std::string& out, F in) { out = std::to_string(in); }
};

export template<typename T, typename F>
    requires std::same_as<T, std::string> && ycore::is_specialization_of_v<F, rc> &&
             convertable<std::string, typename F::element_type>
struct Convert<T, F> {
    static void from(std::string& out, const F& in) { convert(out, *in); }
};

export template<typename T, typename F>
    requires std::same_as<T, std::string> && ycore::is_specialization_of_v<F, up> &&
             convertable<std::string, typename F::element_type>
struct Convert<T, F> {
    static void from(std::string& out, const F& in) { convert(out, *in); }
};

export template<typename T, typename F>
    requires std::same_as<T, std::string_view> && ycore::sized_range<T> &&
             (std::convertible_to<ycore::range_value_t<T>, char> ||
              std::convertible_to<ycore::range_value_t<T>, byte>) &&
             (sizeof(ycore::range_value_t<T>) == sizeof(byte))
struct Convert<T, F> {
    static void from(std::string_view& out, const F& in) {
        out = std::string_view { (const char*)in.data(), (std::size_t)in.size() };
    }
};

export template<typename T, helper::ByteRangeCP Bytes>
    requires std::same_as<T, std::string>
struct Convert<T, Bytes> {
    // already defined by fmt
    constexpr static bool AsFormat { false };
    static void           from(std::string& out, const Bytes& in) {
        out.resize(in.size());
        for (usize i = 0; i < in.size(); i++) {
            out[i] = std::to_integer<char>(in[i]);
        }
    }
};
