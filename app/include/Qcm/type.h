#pragma once

#include <QString>

#include "core/core.h"
#include "core/str_helper.h"
#include "core/fmt.h"

template<>
struct To<QString> {
    template<typename T>
    struct From;

    template<typename T>
    static QString from(const T& t) {
        return From<T>::from(t);
    }

    template<fmt::formattable Fmt>
    struct From<Fmt> {
        static auto from(const Fmt& fmt) { return QString::fromStdString(fmt::format("{}", fmt)); }
    };
};

template<>
struct fmt::formatter<QString> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const QString& qs, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(qs.toStdString(), ctx);
    }
};

inline std::strong_ordering operator<=>(const QString& a, const QString& b) {
    return a < b ? std::strong_ordering::less
                 : (a == b ? std::strong_ordering::equal : std::strong_ordering::greater);
}

template<std::integral T>
struct To<T> {
    template<std::integral F>
    static auto from(F i) {
        return (T)i;
    }
};
