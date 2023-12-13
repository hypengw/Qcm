#pragma once

#include <QVariant>
#include <optional>

#include "core/core.h"

template<typename T>
struct Convert<std::optional<T>, QVariant> {
    using out_type = std::optional<T>;
    using in_type  = QVariant;
    Convert(out_type& o, const in_type& in) {
        o = in.canConvert<T>() ? out_type(in.value<T>()) : std::nullopt;
    }
};