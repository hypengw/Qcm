#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <variant>
#include <iterator>
#include <string_view>
#include <string>
#include <source_location>
#include <format>

import rstd.core;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using idx         = std::ptrdiff_t;
using usize       = std::size_t;
using isize       = std::ptrdiff_t;
using byte        = std::byte;
using voidp       = void*;
using const_voidp = const void*;

template<typename T>
using rc = std::shared_ptr<T>;

template<typename T>
using weak = std::weak_ptr<T>;

template<typename T, typename D = std::default_delete<T>>
using up = std::unique_ptr<T, D>;

template<typename T>
using Arc = std::shared_ptr<T>;

template<typename T, typename D = std::default_delete<T>>
using Box = std::unique_ptr<T, D>;

template<class T, class E>
using Result = rstd::Result<T, E>;

template<typename T>
using Ok = rstd::Ok<T>;
template<typename T>
using Err = rstd::Err<T>;

template<typename U = void, typename T>
constexpr auto Some(T&& t) {
    return rstd::Some<U>(std::forward<T>(t));
}
template<typename U = void, typename T = rstd::option::Unknown>
constexpr auto None(T&& t = {}) {
    return rstd::None<U>(std::forward<T>(t));
}

template<typename T, typename... Args>
auto make_up(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
auto make_box(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
auto make_rc(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
auto make_arc(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
decltype(auto) as_ref(T&& value) {
    if constexpr (std::is_pointer_v<std::remove_reference_t<T>>) {
        return *std::forward<T>(value);
    } else {
        return std::forward<T>(value);
    }
}

struct NoCopy {
protected:
    NoCopy()  = default;
    ~NoCopy() = default;

    NoCopy(const NoCopy&)            = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};

namespace ycore
{
using monostate = std::monostate;

template<typename Wrapper>
typename Wrapper::element_type* GetPtrHelper(const Wrapper& p) {
    return p.get();
}

template<class T>
void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<class T, template<class...> class Primary>
struct is_specialization_of : std::false_type {};
template<template<class...> class Primary, class... Args>
struct is_specialization_of<Primary<Args...>, Primary> : std::true_type {};
template<class T, template<class...> class Primary>
constexpr bool is_specialization_of_v = is_specialization_of<T, Primary>::value;

template<typename T, typename... Ts>
concept convertible_to_any = (std::convertible_to<T, Ts> || ...);

template<typename T, typename F>
concept extra_cvt = (! std::same_as<std::decay_t<T>, std::decay_t<F>>);

template<typename T, typename U = T>
concept has_equal_operator = requires(T a, U b) {
    { a == b } -> std::convertible_to<bool>;
};

template<class T, class U = T>
constexpr bool cmp_exchange(T&  obj,
                            U&& new_value) noexcept(std::is_nothrow_move_constructible<T>::value &&
                                                    std::is_nothrow_assignable<T&, U>::value) {
    if (obj != new_value) {
        obj = std::forward<U>(new_value);
        return true;
    }
    return false;
}

template<class ContainerType>
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

template<typename T>
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
template<typename T>
using range_value_t = std::iter_value_t<T>;

template<typename T>
concept range = requires(T& t) {
    std::begin(t);
    std::end(t);
};

template<class T>
concept sized_range = ycore::range<T> && requires(T& t) { std::size(t); };

template<typename T>
concept tuple_like = detail::is_tuple_like_<T>::value && ! range<T>;

} // namespace ycore

template<typename Tout, typename Tin>
struct Convert;

template<typename Tout, typename Fin>
concept convertable = requires(Tout& t, Fin&& f) {
    { Convert<std::decay_t<Tout>, std::decay_t<Fin>>::from(t, std::forward<Fin>(f)) };
};

template<typename Tout, typename Tin>
    requires convertable<Tout, Tin>
void convert(Tout& out, Tin&& in) {
    Convert<std::decay_t<Tout>, std::decay_t<Tin>>::from(out, std::forward<Tin>(in));
}

template<typename Tout, typename Tin>
    requires convertable<Tout, Tin>
Tout convert_from(Tin&& in) {
    Tout out;
    Convert<std::decay_t<Tout>, std::decay_t<Tin>>::from(out, std::forward<Tin>(in));
    return out;
}

template<typename T>
struct Convert<T, T> {
    static void from(T& out, const T& in) { out = in; }
};

template<std::integral T, std::integral F>
    requires(! std::same_as<T, bool> && ycore::extra_cvt<T, F>) // && (sizeof(T) >= sizeof(F))
struct Convert<T, F> {
    static void from(T& out, F in) { out = (T)in; }
};

template<std::integral T>
    requires ycore::extra_cvt<bool, T>
struct Convert<bool, T> {
    static void from(bool& out, T i) { out = (bool)(i); }
};

template<typename T>
    requires ycore::extra_cvt<ycore::monostate, T>
struct Convert<ycore::monostate, T> {
    static void from(ycore::monostate&, const T&) {};
};

namespace qcm
{

enum class LogLevel
{
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
};
struct LogManager {
    static LogManager* init();
    static LogManager* instance();

    virtual auto level() const -> LogLevel = 0;
    virtual void set_level(LogLevel)       = 0;
};

namespace log
{

auto level_from(std::string_view) -> LogLevel;

void log_loc_raw(LogLevel level, const std::source_location loc, std::string_view);
void log_raw(LogLevel level, std::string_view);

template<typename... T>
void log(LogLevel level, const std::source_location loc, std::format_string<T...> fmt,
         T&&... args) {
    if (level < LogManager::instance()->level()) return;
    log_loc_raw(level, loc, std::vformat(fmt.get(), std::make_format_args(args...)));
}

inline constexpr void             noop() {}
inline constexpr std::string_view noop_str() { return {}; }

[[noreturn]] inline void fail(std::string_view msg) {
    log_raw(LogLevel::ERROR, msg);

    // Crash with access violation and generate crash report.
    volatile auto nullptr_value = (int*)nullptr;
    *nullptr_value              = 0;

    // Silent the possible failure to comply noreturn warning.
    std::abort();
}

constexpr auto enable_debug {
#ifdef NDEBUG
    false
#else
    true
#endif
};

std::string format_assert(std::string_view expr_str, const std::source_location& loc,
                          std::string_view msg);

template<bool Enabled, typename Expr, typename Msg>
    requires Enabled
void handle_assert(Expr&& expr, std::string_view expr_str, Msg&& msg,
                   const std::source_location loc = std::source_location::current()) {
    if (! expr()) {
        fail(format_assert(expr_str, loc, msg()));
    }
}

template<bool Enabled, typename Expr, typename Msg>
    requires(! Enabled)
void handle_assert(Expr&&, std::string_view, Msg&&,
                   const std::source_location = std::source_location::current()) {}

} // namespace log

} // namespace qcm

using namespace std::literals::string_literals;
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

} // namespace helper