#include "asio_qt/qt_sql.h"
#include "json_helper/helper.inl"

namespace helper
{
namespace detail
{
struct Converter {
    SqlConnect::Converter from;
    SqlConnect::Converter to;
};

auto get_converter(int id) -> std::optional<Converter> {
    struct Impl {
        std::map<int, Converter> con;
        std::mutex               mutex;
    };
    static Impl impl { std::map<int, Converter> {
                           { QMetaType::QStringList,
                             {
                                 [](const QVariant& in) -> QVariant {
                                     QStringList out;
                                     auto j = qcm::json::parse(
                                         std::string_view { in.toString().toStdString() });
                                     if (j) {
                                         for (const auto& el : *j.value()) {
                                             out << QString::fromStdString(el.get<std::string>());
                                         }
                                     }
                                     return out;
                                 },
                                 [](const QVariant& in) -> QVariant {
                                     QStringList list = in.toStringList();
                                     qcm::json::njson j;
                                     for (auto& el : list) {
                                         j.push_back(el.toStdString());
                                     }
                                     return QString::fromStdString(j.dump());
                                 },
                             } } },
                       {} };

    std::unique_lock lock { impl.mutex };
    if (auto it = impl.con.find(id); it != impl.con.end()) {
        return it->second;
    }
    return std::nullopt;
}
} // namespace detail

auto SqlConnect::get_from_converter(int id) -> std::optional<Converter> {
    return detail::get_converter(id).transform([](const auto& c) {
        return c.from;
    });
}
auto SqlConnect::get_to_converter(int id) -> std::optional<Converter> {
    return detail::get_converter(id).transform([](const auto& c) {
        return c.to;
    });
}
} // namespace helper
