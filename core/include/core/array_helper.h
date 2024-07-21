#pragma once

#include <array>
#include <ranges>
#include <numeric>
#include "core/core.h"

namespace ycore
{

template<typename Char, usize N1, usize N2>
constexpr auto concat(const Char n1[N1], const Char n2[N2]) -> std::array<Char, N1 + N2> {
    std::array<Char, N1 + N2> out;
    std::copy(n1, n1 + N1, out.begin());
    std::copy(n2, n2 + N2, out.begin() + N1);
    return out;
}

template<typename Char, usize N1, usize N2>
constexpr auto concat(const Char n1[N1], const std::array<Char, N2>& n2)
    -> std::array<Char, N1 + N2> {
    std::array<Char, N1 + N2> out;
    std::copy(n1, n1 + N1, out.begin());
    std::copy(n2.begin(), n2.end(), out.begin() + N1);
    return out;
}

template<typename Char, usize N1, usize N2>
constexpr auto concat(const std::array<Char, N1>& n1, const Char n2[N2])
    -> std::array<Char, N1 + N2> {
    std::array<Char, N1 + N2> out;
    std::copy(n1.begin(), n1.end(), out.begin());
    std::copy(n2, n2 + N2, out.begin() + N1);
    return out;
}

template<typename Char, usize N1, usize N2>
constexpr auto concat(const std::array<Char, N1>& n1, const std::array<Char, N1>& n2)
    -> std::array<Char, N1 + N2> {
    std::array<Char, N1 + N2> out;
    std::copy(n1.begin(), n1.end(), out.begin());
    std::copy(n2.begin(), n2.end(), out.begin() + N1);
    return out;
}

template<typename Char, usize N1, typename... CharIn>
constexpr auto concat(const std::array<Char, N1>& n1, CharIn... chars)
    -> std::array<Char, N1 + sizeof...(CharIn)> {
    std::array<Char, N1 + sizeof...(CharIn)> out;
    std::array<Char, sizeof...(CharIn)>      in { Char(chars)... };
    std::copy(n1.begin(), n1.end(), out.begin());
    std::copy(in.begin(), in.end(), out.begin() + N1);
    return out;
}

template<typename T>
    requires std::ranges::sized_range<T> && std::ranges::sized_range<std::ranges::range_value_t<T>>
constexpr auto join_size(const T& arr, std::string_view del) -> usize {
    return std::accumulate(arr.begin(),
                           arr.end(),
                           (usize)0,
                           [](usize init, const auto& el) {
                               return el.size() + init;
                           }) +
           del.size() * (std::max<usize>(arr.size(), 1) - 1);
}

template<usize N, typename T>
    requires std::ranges::sized_range<T> &&
             std::same_as<std::string_view, std::ranges::range_value_t<T>>
constexpr auto join(const T& arr, std::string_view del) -> std::array<char, N> {
    std::array<char, N> out;
    usize               n { 0 };
    for (auto& el : arr) {
        if (n + el.size() > N) break;
        std::copy(el.begin(), el.end(), out.begin() + n);
        n += el.size();
        if (n + del.size() >= N) break;
        std::copy(del.begin(), del.end(), out.begin() + n);
        n += del.size();
    }
    return out;
};

} // namespace ycore