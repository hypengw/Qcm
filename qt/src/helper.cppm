module;
#include <qtversionchecks.h>
export module qcm.qt:helper;
export import qcm.helper;
export import qt;


using namespace Qt::StringLiterals;

export namespace helper
{

inline bool is_floating_point_metatype_id(int id) {
    switch (id) {
    case QMetaType::Float16:
    case QMetaType::Float:
    case QMetaType::Double: return true;
    default: return false;
    }
}

inline bool is_integer_metatype_id(int id) {
    switch (id) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::UChar:
    case QMetaType::Long:
    case QMetaType::ULong:
    case QMetaType::Bool: return true;
    default: return false;
    }
}

inline bool is_numeric_metatype_id(int id) {
    switch (id) {
    case QMetaType::Float:
    case QMetaType::Double: return true;
    default: return is_integer_metatype_id(id);
    }
}

inline bool is_numeric_metatype(QMetaType type) { return is_numeric_metatype_id(type.id()); }
} // namespace helper

template<typename T, typename F>
    requires cppstd::ranges::range<F> && convertable<T, cppstd::ranges::range_value_t<F>>
struct Convert<QList<T>, F> {
    static void from(QList<T>& out, const F& f) {
        using from_value_type = cppstd::ranges::range_value_t<F>;
        out.clear();
        cppstd::transform(cppstd::ranges::begin(f),
                          cppstd::ranges::end(f),
                          cppstd::back_inserter(out),
                          [](const from_value_type& v) {
                              return convert_from<T>(v);
                          });
    }
};

template<typename T>
    requires rstd::mtp::convertible_to<T, cppstd::string_view> || convertable<cppstd::string_view, T> ||
             convertable<cppstd::string, T>
struct Convert<QString, T> {
    static void from(QString& out, const T& in) {
        if constexpr (rstd::mtp::convertible_to<T, cppstd::string_view>) {
            auto sv = cppstd::string_view(in);
            out     = QString::fromUtf8(sv.data(), sv.size());
        } else if constexpr (convertable<cppstd::string_view, T>) {
            auto sv = convert_from<cppstd::string_view>(in);
            out     = QString::fromUtf8(sv.data(), sv.size());
        } else {
            out = QString::fromStdString(convert_from<cppstd::string>(in));
        }
    }
};

template<>
struct Convert<cppstd::string, QString> {
    static void from(cppstd::string& out, const QString& in) { out = in.toStdString(); }
};

template<>
struct Convert<cppstd::string, QStringView> {
    static void from(cppstd::string& out, QStringView in) { out = in.toString().toStdString(); }
};

template<>
struct Convert<cppstd::string, QLatin1String> {
    static void from(cppstd::string& out, QLatin1String in) { out = in.toString().toStdString(); }
};

template<>
struct Convert<cppstd::string, QUtf8StringView> {
    static void from(cppstd::string& out, QUtf8StringView in) {
        out = cppstd::string_view { in.data(), (usize)in.size() };
    }
};

template<>
struct Convert<cppstd::string, QAnyStringView> {
    static void from(cppstd::string& out, QAnyStringView in) {
        in.visit([&out](auto v) {
            Convert<cppstd::string, decltype(v)>::from(out, v);
        });
    }
};

template<>
struct cppstd::formatter<QString> : cppstd::formatter<cppstd::string_view> {
    template<typename CTX>
    auto format(const QString& qs, CTX& ctx) const -> CTX::iterator {
        return cppstd::formatter<cppstd::string_view>::format(qs.toStdString(), ctx);
    }
};

template<>
struct cppstd::formatter<QStringView> : cppstd::formatter<cppstd::string_view> {
    template<typename CTX>
    auto format(QStringView qs, CTX& ctx) const
        -> CTX::iterator {
        return cppstd::formatter<cppstd::string_view>::format(qs.toString().toStdString(), ctx);
    }
};

template<>
struct cppstd::formatter<QLatin1String> : cppstd::formatter<cppstd::string_view> {
    template<typename CTX>
    auto format(QLatin1String qs, CTX& ctx) const
        -> CTX::iterator {
        return cppstd::formatter<cppstd::string_view>::format(qs.toString().toStdString(), ctx);
    }
};

template<>
struct cppstd::formatter<QUtf8StringView> : cppstd::formatter<cppstd::string_view> {
    template<typename CTX>
    auto format(QUtf8StringView qs, CTX& ctx) const
        -> CTX::iterator {
        return cppstd::formatter<cppstd::string_view>::format({ qs.data(), (usize)qs.size() }, ctx);
    }
};

template<>
struct cppstd::formatter<QAnyStringView> : cppstd::formatter<cppstd::string_view> {
    template<typename CTX>
    auto format(QAnyStringView qs, CTX& ctx) const
        -> CTX::iterator {
        cppstd::string out;
        Convert<cppstd::string, QAnyStringView>::from(out, qs);
        return cppstd::formatter<cppstd::string_view>::format(out, ctx);
    }
};

template<>
struct rstd::Impl<rstd::convert::From<cppstd::string>, QString> {
    static auto from(cppstd::string str) { return QString::fromStdString(rstd::move(str)); }
};
template<>
struct rstd::Impl<rstd::convert::From<cppstd::string>, QUrl> {
    static auto from(cppstd::string str) { return QString::fromStdString(rstd::move(str)); }
};

template<>
struct rstd::Impl<rstd::convert::From<cppstd::string>, QStringView> {
    static auto from(cppstd::string str) { return QString::fromStdString(rstd::move(str)); }
};

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
export inline std::strong_ordering operator<=>(const QString& a, const QString& b) {
    return a < b ? std::strong_ordering::less
                 : (a == b ? std::strong_ordering::equal : std::strong_ordering::greater);
}
#endif

template<typename T>
struct Convert<cppstd::optional<T>, QVariant> {
    using out_type = cppstd::optional<T>;
    using in_type  = QVariant;
    static void from(out_type& o, const in_type& in) {
        o = in.canConvert<T>() ? out_type(in.value<T>()) : cppstd::nullopt;
    }
};

template<typename T>
struct Convert<QVariant, cppstd::optional<T>> {
    using out_type = QVariant;
    using in_type  = cppstd::optional<T>;
    static void from(out_type& o, const in_type& in) {
        if (in) {
            o = out_type::fromValue(in.value());
        }
    }
};