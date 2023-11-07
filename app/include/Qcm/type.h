#pragma once

#include <QString>
#include <QVariant>

#include "core/core.h"
#include "core/str_helper.h"
#include "core/fmt.h"

template<fmt::formattable Fmt>
struct Convert<QString, Fmt> {
    Convert(QString& out, const Fmt& fmt) { out = QString::fromStdString(fmt::format("{}", fmt)); }
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