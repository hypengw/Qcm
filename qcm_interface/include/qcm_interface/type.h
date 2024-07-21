#pragma once

#include <QString>
#include <QVariant>

#include "core/core.h"
#include "core/fmt.h"
#include "core/str_helper.h"
#include "core/qstr_helper.h"

template<typename T, std::integral F>
    requires std::integral<T> && (! std::same_as<T, bool>)
struct Convert<T, F> {
    Convert(T& out, F in) { out = (T)in; }
};

template<std::integral T>
struct Convert<bool, T> {
    Convert(bool& out, T i) { out = (bool)(i); }
};

template<typename T>
struct Convert<QVariant, T> {
    Convert(QVariant& out, const T& in) { out = QVariant::fromValue(in); }
};

inline std::strong_ordering operator<=>(const QVariant& a, const QVariant& b) {
    return a == b    ? std::strong_ordering::equal
           : (a < b) ? std::strong_ordering::less
                     : std::strong_ordering::greater;
}