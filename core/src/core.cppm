module;
#include <cmath>
#include <cstddef>
#include <cstdint>
export module qcm.core:basic;
export import rstd;
import rstd.cppstd;

namespace mtp = rstd::mtp;

export using i8 = std::int8_t;
export using i16 = std::int16_t;
export using i32 = std::int32_t;
export using i64 = std::int64_t;

export using u8 = std::uint8_t;
export using u16 = std::uint16_t;
export using u32 = std::uint32_t;
export using u64 = std::uint64_t;

export using usize = std::size_t;
export using isize = std::ptrdiff_t;
export using rstd::byte;
export using rstd::voidp;
export using rstd::const_voidp;

export template<typename T>
using rc = std::shared_ptr<T>;

export template<typename T>
using weak = std::weak_ptr<T>;

export template<typename T, typename D = std::default_delete<T>>
using up = std::unique_ptr<T, D>;

export using rstd::Option;
export using rstd::Result;
export using rstd::Ok;
export using rstd::Err;
export using rstd::Some;
export using rstd::None;

export using ref_str = rstd::ref<rstd::str>;

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
    if constexpr (mtp::is_ptr<mtp::rm_ref<T>>) {
        return *rstd::forward<T>(value);
    } else {
        return rstd::forward<T>(value);
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

export template<typename T, typename... Ts>
concept convertible_to_any = (mtp::convertible_to<T, Ts> || ...);

export template<typename T, typename F>
concept extra_cvt = (! mtp::same_as<mtp::decay<T>, mtp::decay<F>>);

export template<typename T, typename U = T>
concept has_equal_operator = requires(T a, U b) {
    { a == b } -> mtp::convertible_to<bool>;
};

export template<class T, class U = T>
constexpr bool cmp_exchange(T&  obj,
                            U&& new_value) noexcept(mtp::noex_move<T> && mtp::noex_assign<T&, U>) {
    if (obj != new_value) {
        obj = std::forward<U>(new_value);
        return true;
    }
    return false;
}

export template<typename T>
[[nodiscard]] constexpr bool fuzzy_equal(T a, T b) {
    const T scale = mtp::same_as<T, double> ? T(1000000000000.) : T(100000.f);
    return std::fabs(a - b) * scale <= std::min(std::fabs(a), std::fabs(b));
}

export template<typename T>
using param_t = mtp::cond<mtp::triv_copy<T> && sizeof(T) <= 32, T, const T&>;

export template<typename T>
constexpr auto cmp_set(T&         lhs,
                       param_t<T> rhs) noexcept(std::is_nothrow_move_constructible<T>::value &&
                                                std::is_nothrow_assignable<T&, T>::value)
    -> bool {
    if constexpr (mtp::is_float<T>) {
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

namespace detail
{
template<typename T>
class is_tuple_like_ {
    template<typename U, typename V = typename mtp::rm_cv<U>>
    static auto check(U* p) -> decltype(mtp::tuple_size<V>, 0);
    template<typename>
    static void check(...);

public:
    static constexpr const bool value = ! mtp::is_void<decltype(check<T>(nullptr))>;
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
concept sized_range = ycore::range<T> && requires(T& t) { std::ranges::size(t); };

export template<typename T>
concept tuple_like = detail::is_tuple_like_<T>::value && ! range<T>;

} // namespace ycore

export template<typename Tout, typename Tin>
struct Convert;

export template<typename Tout, typename Fin>
concept convertable = requires(Tout& t, Fin&& f) {
    { Convert<mtp::decay<Tout>, mtp::decay<Fin>>::from(t, rstd::forward<Fin>(f)) };
};

export template<typename Tout, typename Tin>
    requires convertable<Tout, Tin>
void convert(Tout& out, Tin&& in) {
    Convert<mtp::decay<Tout>, mtp::decay<Tin>>::from(out, rstd::forward<Tin>(in));
}

export template<typename Tout, typename Tin>
    requires convertable<Tout, Tin>
Tout convert_from(Tin&& in) {
    Tout out;
    Convert<mtp::decay<Tout>, mtp::decay<Tin>>::from(out, rstd::forward<Tin>(in));
    return out;
}

export template<typename T>
struct Convert<T, T> {
    static void from(T& out, const T& in) { out = in; }
};

export template<mtp::is_int T, mtp::is_int F>
    requires(! mtp::same_as<T, bool> && ycore::extra_cvt<T, F>) // && (sizeof(T) >= sizeof(F))
struct Convert<T, F> {
    static void from(T& out, F in) { out = (T)in; }
};

export template<mtp::is_int T>
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

export template<rstd::mtp::spec_of<rstd::convert::From>  T,
                rstd::mtp::spec_of<rstd::option::Option> A>
    requires rstd::mtp::spec_of<typename T::from_t, std::optional> &&
             rstd::mtp::same_as<typename T::from_t::value_type, typename A::value_type>
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

namespace ycore
{
export template<class T>
    requires(! std::numeric_limits<T>::is_integer)
constexpr auto equal_within_ulps(T x, T y, std::size_t n) -> bool {
    // Since `epsilon()` is the gap size (ULP, unit in the last place)
    // of floating-point numbers in interval [1, 2), we can scale it to
    // the gap size in interval [2^e, 2^{e+1}), where `e` is the exponent
    // of `x` and `y`.

    // If `x` and `y` have different gap sizes (which means they have
    // different exponents), we take the smaller one. Taking the bigger
    // one is also reasonable, I guess.
    const T m = std::min(std::fabs(x), std::fabs(y));

    // Subnormal numbers have fixed exponent, which is `min_exponent - 1`.
    const int exp = m < std::numeric_limits<T>::min() ? std::numeric_limits<T>::min_exponent - 1
                                                      : std::ilogb(m);

    // We consider `x` and `y` equal if the difference between them is
    // within `n` ULPs.
    return std::fabs(x - y) <= n * std::ldexp(std::numeric_limits<T>::epsilon(), exp);
}
} // namespace ycore
