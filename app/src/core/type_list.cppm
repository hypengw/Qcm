module;
#include <type_traits>
export module qcm.core:type_list;
import :basic;

namespace ycore
{
template<typename T, typename... Types>
constexpr usize find_type_in_pack() {
    constexpr usize size        = sizeof...(Types);
    constexpr bool  found[size] = { std::is_same_v<T, Types>... };
    usize           result      = size;
    for (usize i = 0; i < size; ++i) {
        if (found[i]) {
            if (result < size) return size;
            result = i;
        }
    }
    return result;
}

export template<typename... Types>
struct type_list {
    template<template<typename...> class T>
    using to = T<Types...>;

    template<typename T>
    constexpr static usize index() {
        return find_type_in_pack<T, Types...>();
    }
};
} // namespace ycore
