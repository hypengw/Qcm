#pragma once

#include <vector>
#include <algorithm>
#include <ranges>

#include "core/core.h"

template<>
struct To<byte> {
    template<typename T>
        requires std::convertible_to<T, unsigned char>
    static auto from(T c) {
        return byte { (unsigned char)c };
    }
};

template<typename T>
struct To<std::vector<T>> {
    template<typename F>
    static auto from(const F& f);

    template<typename F>
        requires std::ranges::range<F> && to_able<std::ranges::range_value_t<F>, T>
    static auto from(const F& f) {
        using from_value_type = std::ranges::range_value_t<F>;
        std::vector<T> out;
        std::transform(std::ranges::begin(f),
                       std::ranges::end(f),
                       std::back_inserter(out),
                       [](const from_value_type& v) {
                           return To<T>::from(v);
                       });
        return out;
    }
};
