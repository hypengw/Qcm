module;
#include <cstdint>
#include <cstddef>
#include <memory>
#include <optional>
#include <variant>
#include <iterator>
#include <cmath>
#include <expected>

export module qcm.core:basic;
export import rstd;

export using i8  = std::int8_t;
export using i16 = std::int16_t;
export using i32 = std::int32_t;
export using i64 = std::int64_t;

export using u8  = std::uint8_t;
export using u16 = std::uint16_t;
export using u32 = std::uint32_t;
export using u64 = std::uint64_t;

export using idx         = std::ptrdiff_t;
export using usize       = std::size_t;
export using isize       = std::ptrdiff_t;
export using byte        = std::byte;
export using voidp       = void*;
export using const_voidp = const void*;

export template<typename T>
using rc = std::shared_ptr<T>;

export template<typename T>
using weak = std::weak_ptr<T>;

export template<typename T, typename D = std::default_delete<T>>
using up = std::unique_ptr<T, D>;

export template<typename T>
using Arc = std::shared_ptr<T>;

export template<typename T, typename D = std::default_delete<T>>
using Box = std::unique_ptr<T, D>;

export template<typename T, typename E>
using Result = rstd::Result<T, E>;

export template<typename T>
using Option = rstd::Option<T>;

export template<typename T>
using Ok = rstd::Ok<T>;
export template<typename T>
using Err = rstd::Err<T>;

export using ref_str = rstd::ref_str;

export template<typename U = void, typename T>
constexpr auto Some(T&& t) {
    return rstd::Some<U>(std::forward<T>(t));
}
export template<typename U = void, typename T = rstd::option::Unknown>
constexpr auto None(T&& t = {}) {
    return rstd::None<U>(std::forward<T>(t));
}

export template<typename T, typename... Args>
auto make_up(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

export template<typename T, typename... Args>
auto make_box(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

export template<typename T, typename... Args>
auto make_rc(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

export template<typename T, typename... Args>
auto make_arc(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

export template<typename T>
decltype(auto) as_ref(T&& value) {
    if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
        return *std::forward<T>(value);
    } else {
        return std::forward<T>(value);
    }
}

export struct NoCopy {
protected:
    NoCopy()  = default;
    ~NoCopy() = default;

    NoCopy(const NoCopy&)            = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};

namespace ycore
{
export using monostate = std::monostate;

export template<typename Wrapper>
typename Wrapper::element_type* GetPtrHelper(const Wrapper& p) {
    return p.get();
}

export template<class T>
void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

export template<class T, template<class...> class Primary>
struct is_specialization_of : std::false_type {};
export template<template<class...> class Primary, class... Args>
struct is_specialization_of<Primary<Args...>, Primary> : std::true_type {};
export template<class T, template<class...> class Primary>
constexpr bool is_specialization_of_v = is_specialization_of<T, Primary>::value;

export template<typename T, typename... Ts>
concept convertible_to_any = (std::convertible_to<T, Ts> || ...);

export template<typename T, typename F>
concept extra_cvt = (! std::same_as<std::decay_t<T>, std::decay_t<F>>);

export template<typename T, typename U = T>
concept has_equal_operator = requires(T a, U b) {
    { a == b } -> std::convertible_to<bool>;
};

export template<class T, class U = T>
constexpr bool cmp_exchange(T&  obj,
                            U&& new_value) noexcept(std::is_nothrow_move_constructible<T>::value &&
                                                    std::is_nothrow_assignable<T&, U>::value) {
    if (obj != new_value) {
        obj = std::forward<U>(new_value);
        return true;
    }
    return false;
}

export template<typename T>
[[nodiscard]] constexpr bool fuzzy_equal(T a, T b) {
    const T scale = std::same_as<T, double> ? T(1000000000000.) : T(100000.f);
    return std::fabs(a - b) * scale <= std::min(std::fabs(a), std::fabs(b));
}

export template<typename T>
using param_t = std::conditional_t < std::is_trivially_copyable_v<T> && sizeof(T) <= 32,
      T, const T& > ;

export template<typename T>
constexpr auto cmp_set(T&         lhs,
                       param_t<T> rhs) noexcept(std::is_nothrow_move_constructible<T>::value &&
                                                std::is_nothrow_assignable<T&, T>::value) -> bool {
    if constexpr (std::is_floating_point_v<T>) {
        if (! fuzzy_equal(lhs, rhs)) {
            lhs = rhs;
            return true;
        }
    } else {
        if (lhs != rhs) {
            lhs = rhs;
            return true;
        }
    }
    return false;
}
export template<class ContainerType>
concept ContainerConcept = requires(ContainerType a, const ContainerType b) {
    requires std::regular<ContainerType>;
    requires std::swappable<ContainerType>;
    requires std::destructible<typename ContainerType::value_type>;
    requires std::same_as<typename ContainerType::reference, typename ContainerType::value_type&>;
    requires std::same_as<typename ContainerType::const_reference,
                          const typename ContainerType::value_type&>;
    requires std::forward_iterator<typename ContainerType::iterator>;
    requires std::forward_iterator<typename ContainerType::const_iterator>;
    requires std::signed_integral<typename ContainerType::difference_type>;
    requires std::same_as<
        typename ContainerType::difference_type,
        typename std::iterator_traits<typename ContainerType::iterator>::difference_type>;
    requires std::same_as<
        typename ContainerType::difference_type,
        typename std::iterator_traits<typename ContainerType::const_iterator>::difference_type>;
    { a.begin() } -> std::same_as<typename ContainerType::iterator>;
    { a.end() } -> std::same_as<typename ContainerType::iterator>;
    { b.begin() } -> std::same_as<typename ContainerType::const_iterator>;
    { b.end() } -> std::same_as<typename ContainerType::const_iterator>;
    { a.cbegin() } -> std::same_as<typename ContainerType::const_iterator>;
    { a.cend() } -> std::same_as<typename ContainerType::const_iterator>;
    { a.size() } -> std::same_as<typename ContainerType::size_type>;
    { a.max_size() } -> std::same_as<typename ContainerType::size_type>;
    { a.empty() } -> std::same_as<bool>;
};

export template<typename T>
concept MapConcept = requires(T t) {
    requires ContainerConcept<T>;
    typename T::key_type;
    typename T::mapped_type;
};

namespace detail
{
template<typename T>
class is_tuple_like_ {
    template<typename U, typename V = typename std::remove_cv<U>::type>
    static auto check(U* p) -> decltype(std::tuple_size<V>::value, 0);
    template<typename>
    static void check(...);

public:
    static constexpr const bool value = ! std::is_void<decltype(check<T>(nullptr))>::value;
};
} // namespace detail
export template<typename T>
using range_value_t = std::iter_value_t<T>;

export template<typename T>
concept range = requires(T& t) {
    std::begin(t);
    std::end(t);
};

export template<class T>
concept sized_range = ycore::range<T> && requires(T& t) { std::size(t); };

export template<typename T>
concept tuple_like = detail::is_tuple_like_<T>::value && ! range<T>;

} // namespace ycore

export template<typename Tout, typename Tin>
struct Convert;

export template<typename Tout, typename Fin>
concept convertable = requires(Tout& t, Fin&& f) {
    { Convert<std::decay_t<Tout>, std::decay_t<Fin>>::from(t, std::forward<Fin>(f)) };
};

export template<typename Tout, typename Tin>
    requires convertable<Tout, Tin>
void convert(Tout& out, Tin&& in) {
    Convert<std::decay_t<Tout>, std::decay_t<Tin>>::from(out, std::forward<Tin>(in));
}

export template<typename Tout, typename Tin>
    requires convertable<Tout, Tin>
Tout convert_from(Tin&& in) {
    Tout out;
    Convert<std::decay_t<Tout>, std::decay_t<Tin>>::from(out, std::forward<Tin>(in));
    return out;
}

export template<typename T>
struct Convert<T, T> {
    static void from(T& out, const T& in) { out = in; }
};

export template<std::integral T, std::integral F>
    requires(! std::same_as<T, bool> && ycore::extra_cvt<T, F>) // && (sizeof(T) >= sizeof(F))
struct Convert<T, F> {
    static void from(T& out, F in) { out = (T)in; }
};

export template<std::integral T>
    requires ycore::extra_cvt<bool, T>
struct Convert<bool, T> {
    static void from(bool& out, T i) { out = (bool)(i); }
};

export template<typename T>
    requires ycore::extra_cvt<ycore::monostate, T>
struct Convert<ycore::monostate, T> {
    static void from(ycore::monostate&, const T&) {};
};

export template<typename IMPL>
struct CRTP {
protected:
    IMPL&       crtp_impl() { return *static_cast<IMPL*>(this); }
    const IMPL& crtp_impl() const { return *static_cast<const IMPL*>(this); }
};

export template<rstd::meta::special_of<rstd::convert::From>  T,
                rstd::meta::special_of<rstd::option::Option> A>
    requires rstd::meta::special_of<typename T::from_t, std::optional> &&
             std::same_as<typename T::from_t::value_type, typename A::value_type>
struct rstd::Impl<T, A> {
    using Self = A;
    static auto from(typename T::from_t value) -> Self {
        if (value) {
            return rstd::Some(std::move(*value));
        } else {
            return rstd::None();
        }
    }
};
