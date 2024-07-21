#pragma once

#include <vector>
#include <algorithm>
#include <ranges>

#include "core/core.h"

template<typename T>
    requires std::convertible_to<T, unsigned char>
struct Convert<byte, T> {
    static void from(byte& out, T c) { out = byte { (unsigned char)c }; }
};

template<typename T, typename F>
    requires std::ranges::range<F> && convertable<T, std::ranges::range_value_t<F>>
struct Convert<std::vector<T>, F> {
    static void from(std::vector<T>& out, const F& f) {
        using from_value_type = std::ranges::range_value_t<F>;
        out.clear();
        std::transform(std::ranges::begin(f),
                       std::ranges::end(f),
                       std::back_inserter(out),
                       [](const from_value_type& v) {
                           return convert_from<T>(v);
                       });
    }
};