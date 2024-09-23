#pragma once

#include <numeric>
#include <utility>
#include <type_traits>
#include "core/core.h"

namespace ycore
{

// forward
template<typename... TS>
struct type_list;

namespace detail
{

// must retrun
template<typename Func, usize I, typename T, typename... Ts>
auto runtime_select(usize i, Func&& func) {
    if constexpr (sizeof...(Ts) > 0) {
        return i == I ? func.template operator()<I, T>()
                      : runtime_select<Func, I + 1, Ts...>(i, std::forward<Func>(func));
    } else {
        return i == I ? func.template operator()<I, T>()
                      : func.template operator()<I, std::monostate>();
    }
}

// template<usize Idx, typename T, typename... Ts>
// struct get_nth_type;
//
// template<typename T, typename... Ts>
// struct get_nth_type<0, T, Ts...> {
//     using type = T;
// };
//
// template<usize Idx, typename T, typename... Ts>
// struct get_nth_type {
//     using type = get_nth_type<Idx - 1, Ts...>::type;
// };

template<usize I, typename T, typename... TS>
struct split;

template<typename T>
struct split<0, T> {
    using nth    = T;
    using before = type_list<>;
    using after  = type_list<>;
};

template<typename T, typename... TS>
struct split<0, T, TS...> {
    using nth    = T;
    using before = type_list<>;
    using after  = type_list<TS...>;
};

template<typename T, typename... TS>
struct split<1, T, TS...> {
    using nth    = split<0, TS...>::nth;
    using before = type_list<T>;
    using after  = split<0, TS...>::after;
};

template<usize I, typename T, typename... TS>
struct split {
    using nth    = split<I - 1, TS...>::nth;
    using before = split<I - 1, TS...>::before::template prepend<T>;
    using after  = split<I - 1, TS...>::after;
};

template<usize I, typename... TS>
struct split_ {
    using nth    = split<I, TS...>::nth;
    using before = split<I, TS...>::before;
    using after  = split<I, TS...>::after;
};

template<typename T, typename... Ts>
struct one {
    using type = type_list<T>;
};

template<typename T, typename... TS>
struct extend {
    static_assert(false);
};

template<typename... TS, typename... TS2>
struct extend<type_list<TS...>, type_list<TS2...>> {
    using type = type_list<TS..., TS2...>;
};

template<typename... TS, typename... T>
struct extend<type_list<TS...>, T...> {
    using type = extend<type_list<TS...>, typename extend<T...>::type>::type;
};

template<typename T, typename... TS>
struct intersection {
    static_assert(false);
};

template<typename... TS, typename... TS2>
struct intersection<type_list<TS...>, type_list<TS2...>> {
    using A    = type_list<TS...>;
    using type = extend<
        std::conditional_t<(A::template contains<TS2>()), type_list<TS2>, type_list<>>...>::type;
};

template<typename... TS, typename... T>
struct intersection<type_list<TS...>, T...> {
    using type = intersection<type_list<TS...>, typename intersection<T...>::type>::type;
};

template<typename T>
struct index_sequence_helper;

template<std::size_t... IS>
struct index_sequence_helper<std::integer_sequence<std::size_t, IS...>> {
    template<template<std::size_t I, typename Arg> class F, typename... T>
    using test = std::integral_constant<bool, (F<IS, T>::value && ...)>;
};

} // namespace detail

template<std::size_t N>
using make_index_sequence_helper = detail::index_sequence_helper<std::make_index_sequence<N>>;

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

template<typename... TS>
struct type_list {
    // Compile time operations
    template<template<typename> class W>
    using wrap = type_list<W<TS>...>;

    template<template<typename> class M>
    using map = type_list<typename M<TS>::type...>;

    template<typename... TS2>
    using append = type_list<TS..., TS2...>;

    template<typename... TS2>
    using prepend = type_list<TS2..., TS...>;

    template<typename T>
    using extend = detail::extend<T, type_list<TS...>>::type;

    template<typename T>
    using intersection = detail::intersection<T, type_list<TS...>>::type;

    template<usize Idx>
    using before = detail::split_<Idx, TS...>::before;

    template<usize Idx>
    using after = detail::split_<Idx, TS...>::after;

    template<usize I>
    using erase = detail::extend<before<I>, after<I>>::type;

    template<typename T>
    constexpr static auto contains() -> bool {
        return (std::same_as<T, TS> || ...);
    }

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
    using at = detail::split_<I, TS...>::nth;

    template<typename Func>
        requires requires(Func f) { f.template operator()<0, at<0>>(); }
    static auto runtime_select(usize idx, Func&& func) {
        return detail::runtime_select<Func, 0, TS...>(idx, std::forward<Func>(func));
    }
};

template<typename T>
struct get_type_list {
    using type = type_list<T>;
};
template<template<typename...> class T, typename... TS>
struct get_type_list<T<TS...>> {
    using type = type_list<TS...>;
};

template<typename T>
using get_type_list_t = get_type_list<T>::type;

static_assert(std::same_as<float, type_list<int, float, bool>::at<1>>);
static_assert(std::same_as<type_list<int, bool>, type_list<int, float, bool>::erase<1>>);
static_assert(std::same_as<type_list<float, bool>, type_list<int, float, bool>::erase<0>>);
static_assert(std::same_as<type_list<int, float>, type_list<int, float, bool>::erase<2>>);
static_assert(std::same_as<type_list<>, type_list<int>::erase<0>>);

static_assert(
    std::same_as<type_list<int, float>::intersection<type_list<float>>, type_list<float>>);
} // namespace ycore