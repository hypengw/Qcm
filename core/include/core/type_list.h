#pragma once

#include <numeric>
#include "core/core.h"

namespace ycore
{

namespace detail
{

template<typename Func, usize I, typename T, typename... Ts>
auto runtime_select(usize i, Func&& func) {
    if constexpr (sizeof...(Ts) > 0) {
        return i == I ? func.template operator()<I, T>()
                      : runtime_select<Func, I + 1, Ts...>(i, std::forward<Func>(func));
    } else {
        return func.template operator()<I, T>();
    }
}

template<usize Idx, typename T, typename... Ts>
struct get_nth_type;

template<typename T, typename... Ts>
struct get_nth_type<0, T, Ts...> {
    using type = T;
};

template<usize Idx, typename T, typename... Ts>
struct get_nth_type {
    using type = get_nth_type<Idx - 1, Ts...>::type;
};

} // namespace detail

template<typename _Tp, typename... _Types>
constexpr usize find_type_in_pack() {
    constexpr usize sz        = sizeof...(_Types);
    constexpr bool  found[sz] = { std::is_same_v<_Tp, _Types>... };
    usize           n         = sz;
    for (usize i = 0; i < sz; ++i) {
        if (found[i]) {
            if (n < sz) // more than one _Tp found
                return sz;
            n = i;
        }
    }
    return n;
}

// forward
template<typename... TS>
struct type_list;

template<typename... TS>
struct type_list {
    // Compile time operations
    template<template<typename> class W>
    using wrap = type_list<W<TS>...>;

    template<template<typename> class M>
    using map = type_list<typename M<TS>::type...>;

    template<typename T>
    using append = type_list<TS..., T>;

    template<typename T>
    using prepend = type_list<T, TS...>;

    template<template<typename...> class T>
    using to = T<TS...>;

    using iterator = idx;
    static constexpr auto size() { return sizeof...(TS); }
    static constexpr auto begin() { return iterator { 0 }; }
    static constexpr auto end() { return iterator { size() }; }

    template<typename T>
    constexpr static usize index() {
        return find_type_in_pack<T, TS...>();
    }

    template<usize I>
    using at = detail::get_nth_type<I, TS...>::type;

    template<typename Func>
        requires requires(Func f) { f.template operator()<0, at<0>>(); }
    static auto runtime_select(usize idx, Func&& func) {
        return detail::runtime_select<Func, 0, TS...>(idx, std::forward<Func>(func));
    }
};

} // namespace core