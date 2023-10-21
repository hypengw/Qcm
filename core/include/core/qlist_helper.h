#pragma once

#include "core/vec_helper.h"

#include <QList>

template<typename T>
struct To<QList<T>> {
    template<typename F>
    static auto from(const F& f);

    template<typename F>
        requires std::ranges::range<F> && to_able<std::ranges::range_value_t<F>, T>
    static auto from(const F& f) {
        using from_value_type = std::ranges::range_value_t<F>;
        QList<T> out;
        std::transform(std::ranges::begin(f),
                       std::ranges::end(f),
                       std::back_inserter(out),
                       [](const from_value_type& v) {
                           return To<T>::from(v);
                       });
        return out;
    }
};