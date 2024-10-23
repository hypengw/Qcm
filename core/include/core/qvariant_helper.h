#pragma once

#include <QVariant>
#include <optional>

#include "core/core.h"

template<typename T>
struct Convert<std::optional<T>, QVariant> {
    using out_type = std::optional<T>;
    using in_type  = QVariant;
    static void from(out_type& o, const in_type& in) {
        o = in.canConvert<T>() ? out_type(in.value<T>()) : std::nullopt;
    }
};

template<typename T>
struct Convert<QVariant, std::optional<T>> {
    using out_type = QVariant;
    using in_type  = std::optional<T>;
    static void from(out_type& o, const in_type& in) {
        if (in) {
            o = out_type::fromValue(in.value());
        }
    }
};