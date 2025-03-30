#pragma once
#include <QString>
#include <QStringView>
#include <QVariant>
#include <QUrl>

#include "core/core.h"
#include "core/fmt.h"

using namespace Qt::StringLiterals;

template<typename T>
    requires std::convertible_to<T, std::string_view> || convertable<std::string_view, T> ||
             convertable<std::string, T>
struct Convert<QString, T> {
    static void from(QString& out, const T& in) {
        if constexpr (std::convertible_to<T, std::string_view>) {
            auto sv = std::string_view(in);
            out     = QString::fromUtf8(sv.data(), sv.size());
        } else if constexpr (convertable<std::string_view, T>) {
            auto sv = convert_from<std::string_view>(in);
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
    static void from(std::string& out, QStringView in) { out = in.toString().toStdString(); }
};

template<>
struct Convert<std::string, QLatin1String> {
    static void from(std::string& out, QLatin1String in) { out = in.toString().toStdString(); }
};

template<>
struct Convert<std::string, QUtf8StringView> {
    static void from(std::string& out, QUtf8StringView in) {
        out = std::string_view { in.data(), (usize)in.size() };
    }
};

template<>
struct Convert<std::string, QAnyStringView> {
    static void from(std::string& out, QAnyStringView in) {
        in.visit([&out](auto v) {
            Convert<std::string, decltype(v)>::from(out, v);
        });
    }
};

template<>
struct fmt::formatter<QString> : fmt::formatter<std::string_view> {
    auto format(const QString& qs, fmt::format_context& ctx) const
        -> fmt::format_context::iterator {
        return fmt::formatter<std::string_view>::format(qs.toStdString(), ctx);
    }
};

template<>
struct fmt::formatter<QStringView> : fmt::formatter<std::string_view> {
    auto format(QStringView qs, fmt::format_context& ctx) const -> fmt::format_context::iterator {
        return fmt::formatter<std::string_view>::format(qs.toString().toStdString(), ctx);
    }
};

template<>
struct fmt::formatter<QLatin1String> : fmt::formatter<std::string_view> {
    auto format(QLatin1String qs, fmt::format_context& ctx) const -> fmt::format_context::iterator {
        return fmt::formatter<std::string_view>::format(qs.toString().toStdString(), ctx);
    }
};

template<>
struct fmt::formatter<QUtf8StringView> : fmt::formatter<std::string_view> {
    auto format(QUtf8StringView qs, fmt::format_context& ctx) const
        -> fmt::format_context::iterator {
        return fmt::formatter<std::string_view>::format({ qs.data(), (usize)qs.size() }, ctx);
    }
};

template<>
struct fmt::formatter<QAnyStringView> : fmt::formatter<std::string_view> {
    auto format(QAnyStringView qs, fmt::format_context& ctx) const
        -> fmt::format_context::iterator {
        std::string out;
        Convert<std::string, QAnyStringView>::from(out, qs);
        return fmt::formatter<std::string_view>::format(out, ctx);
    }
};

template<>
struct std::formatter<QString> : std::formatter<std::string_view> {
    auto format(const QString& qs, std::format_context& ctx) const
        -> std::format_context::iterator {
        return std::formatter<std::string_view>::format(qs.toStdString(), ctx);
    }
};

template<>
struct std::formatter<QStringView> : std::formatter<std::string_view> {
    auto format(QStringView qs, std::format_context& ctx) const -> std::format_context::iterator {
        return std::formatter<std::string_view>::format(qs.toString().toStdString(), ctx);
    }
};

template<>
struct std::formatter<QLatin1String> : std::formatter<std::string_view> {
    auto format(QLatin1String qs, std::format_context& ctx) const -> std::format_context::iterator {
        return std::formatter<std::string_view>::format(qs.toString().toStdString(), ctx);
    }
};

template<>
struct std::formatter<QUtf8StringView> : std::formatter<std::string_view> {
    auto format(QUtf8StringView qs, std::format_context& ctx) const
        -> std::format_context::iterator {
        return std::formatter<std::string_view>::format({ qs.data(), (usize)qs.size() }, ctx);
    }
};

template<>
struct std::formatter<QAnyStringView> : std::formatter<std::string_view> {
    auto format(QAnyStringView qs, std::format_context& ctx) const
        -> std::format_context::iterator {
        std::string out;
        Convert<std::string, QAnyStringView>::from(out, qs);
        return std::formatter<std::string_view>::format(out, ctx);
    }
};

template<>
struct rstd::Impl<rstd::convert::From<std::string>, QString> {
    static auto from(std::string str) { return QString::fromStdString(std::move(str)); }
};
template<>
struct rstd::Impl<rstd::convert::From<std::string>, QUrl> {
    static auto from(std::string str) { return QString::fromStdString(std::move(str)); }
};

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
inline std::strong_ordering operator<=>(const QString& a, const QString& b) {
    return a < b ? std::strong_ordering::less
                 : (a == b ? std::strong_ordering::equal : std::strong_ordering::greater);
}
#endif