module;
#include <vector>
#include <algorithm>

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

