module;
#include <cmath>
#include <cstddef>
#include <cstdint>
export module qcm.core:basic;
import rstd;
import rstd.cppstd;

namespace mtp = rstd::mtp;

export using i32   = std::int32_t;
export using i64   = std::int64_t;
export using u8    = std::uint8_t;
export using u32   = std::uint32_t;
export using usize = std::size_t;

export template<typename T>
using rc = std::shared_ptr<T>;

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

export namespace ycore
{
template<typename Wrapper>
typename Wrapper::element_type* GetPtrHelper(const Wrapper& p) {
    return p.get();
}

template<typename T>
[[nodiscard]] constexpr bool fuzzy_equal(T a, T b) {
    const T scale = mtp::same_as<T, double> ? T(1000000000000.) : T(100000.f);
    return std::fabs(a - b) * scale <= std::min(std::fabs(a), std::fabs(b));
}

template<typename T>
using param_t = mtp::cond<mtp::triv_copy<T> && sizeof(T) <= 32, T, const T&>;

template<typename T>
constexpr auto cmp_set(T&         lhs,
                       param_t<T> rhs) noexcept(std::is_nothrow_move_constructible<T>::value &&
                                                std::is_nothrow_assignable<T&, T>::value) -> bool {
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
} // namespace ycore
