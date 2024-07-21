#pragma once
#include <QString>
#include <QVariant>

#include "core/core.h"
#include "core/fmt.h"


template<fmt::formattable Fmt>
struct Convert<QString, Fmt> {
    static void from(QString& out, const Fmt& fmt) { out = QString::fromStdString(fmt::format("{}", fmt)); }
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