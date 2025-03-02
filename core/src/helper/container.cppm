module;
#include <vector>
#include <variant>
#include <algorithm>
#include <optional>

export module qcm.helper:container;
export import qcm.core;

namespace helper
{
export template<typename T>
struct array_traits {
    static_assert(false);
};
template<typename T, std::size_t N>
struct array_traits<T[N]> {
    using type                 = T;
    static constexpr auto size = N;
};

} // namespace helper

export template<>
struct Convert<byte, unsigned char> {
    static void from(byte& out, unsigned char c) { out = byte { (unsigned char)c }; }
};

export template<typename T, typename F>
    requires(std::same_as<T, byte> && std::convertible_to<F, unsigned char>)
struct Convert<T, F> {
    static void from(byte& out, F c) { out = byte { (unsigned char)c }; }
};

export template<typename T, typename F>
    requires ycore::is_specialization_of_v<T, std::vector> && ycore::range<F> &&
             convertable<ycore::range_value_t<T>, ycore::range_value_t<F>>
struct Convert<T, F> {
    static void from(T& out, const F& f) {
        using to_value_type   = ycore::range_value_t<T>;
        using from_value_type = ycore::range_value_t<F>;
        out.clear();
        std::transform(
            std::begin(f), std::end(f), std::back_inserter(out), [](const from_value_type& v) {
                return convert_from<to_value_type>(v);
            });
    }
};

// variant ---------------------

export template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
export template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace helper
{

namespace detail
{

template<typename T, std::size_t I, std::size_t... Is>
auto make_variant(const std::size_t i, std::index_sequence<I, Is...>) -> T {
    if constexpr (sizeof...(Is) > 0) {
        return i == I ? T(std::in_place_index<I>)
                      : make_variant<T>(i, std::index_sequence<Is...> {});
    } else {
        return T(std::in_place_index<I>);
    }
}

} // namespace detail

export template<typename... Ts>
auto make_variant(const std::size_t i) -> std::variant<Ts...> {
    return detail::make_variant<std::variant<Ts...>>(i, std::index_sequence_for<Ts...> {});
}

export template<typename T, typename... Types>
constexpr auto to_optional(const std::variant<Types...>& v) noexcept -> std::optional<T> {
    if (const T* p = std::get_if<T>(&v))
        return *p;
    else
        return std::nullopt;
}

export template<typename... ToTs, typename... InTs>
bool variant_convert(std::variant<ToTs...>& out, const std::variant<InTs...>& in) {
    return std::visit(
        overloaded { [&out]<typename T>(const T& in) -> bool
                         requires(ycore::type_list<ToTs...>::template contains<T>())
                                 {
                                     out = in;
                                     return true;
                                 },
                                 []<typename T>(const T&) -> bool
                                     requires(! ycore::type_list<ToTs...>::template contains<T>())
                     {
                         return false;
                     } },
                     in);
}

} // namespace helper