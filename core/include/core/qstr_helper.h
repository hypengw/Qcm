#pragma once
#include <QString>
#include <QVariant>

#include "core/core.h"
#include "core/fmt.h"

template<typename T>
    requires std::convertible_to<T, std::string_view> || convertable<std::string, T>
struct Convert<QString, T> {
    static void from(QString& out, const T& in) {
        if constexpr (std::convertible_to<T, std::string_view>) {
            auto sv = std::string_view(in);
            out     = QString::fromUtf8(sv.data(), sv.size());
        } else {
            out = QString::fromStdString(convert_from<std::string>(in));
        }
    }
};

template<>
struct Convert<std::string, QString> {
    static void from(std::string& out, const QString& in) { out = in.toStdString(); }
};

template<>
struct Convert<std::string, QStringView> {
    static void from(std::string& out, const QStringView& in) { out = in.toString().toStdString(); }
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