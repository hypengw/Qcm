#include "asio_qt/qt_sql.h"
#include "json_helper/helper.inl"
#include "Qcm/query/query_model.h"

#include <QDateTime>

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
    const static Impl impl {
        std::map<int, Converter> {
            { QMetaType::fromType<std::vector<QString>>().id(),
              {
                  [](const QVariant& in) -> QVariant {
                      std::vector<QString> out;
                      auto j = qcm::json::parse(std::string_view { in.toString().toStdString() });
                      if (j) {
                          for (const auto& el : *j.value()) {
                              out.emplace_back(QString::fromStdString(el.get<std::string>()));
                          }
                      }
                      return QVariant::fromValue(out);
                  },
                  [](const QVariant& in) -> QVariant {
                      auto             list = in.value<std::vector<QString>>();
                      qcm::json::njson j;
                      for (auto& el : list) {
                          j.push_back(el.toStdString());
                      }
                      return QString::fromStdString(j.dump());
                  },
              } },
            { QMetaType::QStringList,
              {
                  [](const QVariant& in) -> QVariant {
                      QStringList out;
                      auto j = qcm::json::parse(std::string_view { in.toString().toStdString() });
                      if (j) {
                          for (const auto& el : *j.value()) {
                              out << QString::fromStdString(el.get<std::string>());
                          }
                      }
                      return out;
                  },
                  [](const QVariant& in) -> QVariant {
                      QStringList      list = in.toStringList();
                      qcm::json::njson j;
                      for (auto& el : list) {
                          j.push_back(el.toStdString());
                      }
                      return QString::fromStdString(j.dump());
                  },
              } },
            { QMetaType::QDateTime,
              {
                  [](const QVariant& in) -> QVariant {
                      return in.toDateTime();
                  },
                  [](const QVariant& in) -> QVariant {
                      auto date = in.toDateTime();
                      if (date.isNull()) {
                          // no null
                          return QDateTime::fromSecsSinceEpoch(0);
                      }
                      return date;
                  },
              } },
            { qcm::model::ItemId::staticMetaObject.metaType().id(),
              {
                  [](const QVariant& in) -> QVariant {
                      return QVariant::fromValue(qcm::model::ItemId(in.toUrl()));
                  },
                  [](const QVariant& in) -> QVariant {
                      return in.value<qcm::model::ItemId>().toUrl();
                  },
              } },
        },

        {}
    };

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

namespace qcm
{
auto query::get_from_converter(int id) -> std::optional<Converter> {
    return helper::detail::get_converter(id).transform([](const auto& c) {
        return c.from;
    });
}
auto query::get_to_converter(int id) -> std::optional<Converter> {
    return helper::detail::get_converter(id).transform([](const auto& c) {
        return c.to;
    });
}
} // namespace qcm
