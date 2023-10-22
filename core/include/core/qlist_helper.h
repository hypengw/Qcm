#pragma once

#include "core/vec_helper.h"

#include <QList>

template<typename T, typename F>
    requires std::ranges::range<F> && convertable<T, std::ranges::range_value_t<F>>
struct Convert<QList<T>, F> {
    Convert(QList<T>& out, const F& f) {
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