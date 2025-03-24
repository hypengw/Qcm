#pragma once

#include <cstdint>
#include <cstddef>
#include <utility>
#include <system_error>

namespace meta_model
{

using i32   = std::int32_t;
using usize = std::size_t;
template<typename T>
using param_type = std::conditional_t<std::is_trivially_copyable_v<T>, T, const T&>;

template<class T, template<class...> class Primary>
struct is_specialization_of : std::false_type {};

template<template<class...> class Primary, class... Args>
struct is_specialization_of<Primary<Args...>, Primary> : std::true_type {};

template<class T, template<class...> class Primary>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Primary>::value;

///
/// @brief Trait for Item behavior
/// @code {.cpp}
/// // T: T or const T&
/// auto hash(T) noexcept -> usize;
/// auto compare_lt(T, T) noexcept -> usize;
/// @endcode
/// @tparam Item type
template<typename T>
struct ItemTrait;

///
/// @brief Item that defined hash in ItemTrait
template<typename T>
concept hashable_item = std::semiregular<ItemTrait<T>> && requires(T t) {
    { ItemTrait<T>::hash(t) } -> std::same_as<usize>;
};

///
/// @brief Item that defined compare_lt in ItemTrait
template<typename T>
concept comparable_item = std::semiregular<ItemTrait<T>> && requires(T t) {
    { ItemTrait<T>::compare_lt(t, t) } -> std::same_as<bool>;
};

template<typename T>
    requires std::is_arithmetic_v<T>
struct ItemTrait<T> {
    static auto hash(T v) noexcept -> usize { return std::hash<T> {}(v); }
    static auto compare_lt(T a, T b) noexcept -> usize { return a < b; }
};

} // namespace meta_model