#pragma once

#include <filesystem>
#include <set>
#include <ranges>

#include <QtCore/QThread>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include <asio/dispatch.hpp>

#include "core/log.h"
#include "core/strv_helper.h"
#include "core/qstr_helper.h"
#include "core/qmeta_helper.h"
#include "asio_qt/qt_executor.h"

namespace helper
{

class SqlConnect : public std::enable_shared_from_this<SqlConnect> {
public:
    SqlConnect(const std::filesystem::path& path, QStringView name)
        : m_db_path(path),
          m_name(name),
          m_thread(),
          m_ctx(make_rc<QtExecutionContext>(&m_thread, (QEvent::Type)QEvent::registerEventType())),
          m_ex(m_ctx) {
        m_thread.start();
        asio::dispatch(m_ex, [this]() {
            connect_db();
        });
    }
    ~SqlConnect() {
        m_thread.quit();
        m_thread.wait();
    }

    class Query : public QSqlQuery {
    public:
        Query(const QSqlDatabase& db): QSqlQuery(db), m_thread(db.thread()) {
            _assert_rel_(m_thread == nullptr || m_thread->isCurrentThread());
        }
        ~Query() { _assert_rel_(m_thread == nullptr || m_thread->isCurrentThread()); }
        Query(const Query&)            = delete;
        Query& operator=(const Query&) = delete;
        Query(Query&& o): QSqlQuery(std::move(o)), m_thread(std::exchange(o.m_thread, nullptr)) {}
        Query& operator=(Query&& o) {
            QSqlQuery::operator=(std::move(o));
            m_thread = std::exchange(o.m_thread, nullptr);
            return *this;
        }

    private:
        QThread* m_thread;
    };

    using Converter = std::function<QVariant(const QVariant&)>;
    static auto get_from_converter(int id) -> std::optional<Converter>;
    static auto get_to_converter(int id) -> std::optional<Converter>;

    static constexpr auto EditTimeColumn { "_editTime DATETIME DEFAULT CURRENT_TIMESTAMP"sv };
    static constexpr QStringView EditTimeColumnQSV {
        u"_editTime DATETIME DEFAULT CURRENT_TIMESTAMP"
    };

    auto get_executor() -> QtExecutor& { return m_ex; }

    auto is_open() const -> bool { return m_db.isOpen(); }

    auto query() const -> Query { return Query(m_db); }
    auto db() -> QSqlDatabase& { return m_db; }
    auto error_str() -> QString { return m_db.lastError().text(); }

    auto generate_column_migration(QStringView table_name, QStringView create_sql,
                                   std::span<const std::string> req_columns) -> QStringList {
        QSqlQuery query(m_db);
        QString   pragmaQuery = QString("PRAGMA table_info(%1);").arg(table_name);

        std::set<std::string, std::less<>> columns;

        if (query.exec(pragmaQuery)) {
            while (query.next()) {
                auto name = query.value(1).toString().toStdString();
                columns.insert(name);
            }
        }

        if (columns.size() > 0) {
            std::set<std::string> common_columns;
            for (auto& el : req_columns) {
                if (columns.contains(el)) {
                    common_columns.insert(std::string(el));
                }
            }
            if (common_columns.size() < req_columns.size()) {
                auto columns_str =
                    QString::fromStdString(fmt::format("{}", fmt::join(common_columns, ", ")));

                QStringList out;
                out << u"ALTER TABLE %1 RENAME TO %1_backup;"_s.arg(table_name);
                out << create_sql.toString();
                out << u"INSERT INTO %1 (%2) SELECT %2 FROM %1_backup;"_s.arg(table_name,
                                                                              columns_str);
                out << u"DROP TABLE %1_backup;"_s.arg(table_name);
                return out;
            }
        }
        return { create_sql.toString() };
    }

    auto generate_column_migration(QStringView table_name, QStringView primary,
                                   const QMetaObject&                meta,
                                   std::span<const std::string_view> extras,
                                   std::set<std::string_view>        exclude = {}) {
        std::vector<std::string> column_names;
        std::vector<std::string> columns;
        bool                     primary_ok { false };
        std::string_view         primary_str { " PRIMARY KEY" };
        for (int i = 0; i < meta.propertyCount(); i++) {
            const auto& p    = meta.property(i);
            auto        name = p.name();
            if (exclude.contains(name)) {
                continue;
            }

            std::string_view type;
            if (p.typeId() == qMetaTypeId<QString>()) {
                type = "TEXT"sv;
            } else if (helper::is_integer_metatype_id(p.typeId())) {
                type = "INTEGER"sv;
            } else if (helper::is_floating_point_metatype_id(p.typeId())) {
                type = "REAL"sv;
            } else if (p.typeId() == qMetaTypeId<QDateTime>()) {
                type = "DATETIME"sv;
            } else {
                type = "TEXT"sv;
            }
            if (name == primary) {
                primary_ok = true;
            }
            columns.push_back(
                fmt::format("{} {}{}", name, type, name == primary ? primary_str : ""sv));
            column_names.push_back(name);
        }
        _assert_rel_(primary_ok);
        for (auto& el : extras) {
            auto s = std::string(el);
            columns.push_back(s);
            column_names.push_back(s.substr(0, s.find_first_of(' ')));
        }

        columns.emplace_back(EditTimeColumn);
        column_names.emplace_back(EditTimeColumn.substr(0, EditTimeColumn.find_first_of(' ')));

        return generate_column_migration(
            table_name,
            QString::fromStdString(fmt::format(R"(
CREATE TABLE IF NOT EXISTS {} (
{}
);
)",
                                               table_name,
                                               fmt::join(columns, ",\n"))),
            column_names);
    }

    struct InsertHelper {
        QString                         sql;
        std::map<QString, QVariantList> binds;
        void                            bind(QSqlQuery& q) {
            q.prepare(sql);
            for (auto& b : binds) {
                q.bindValue(u":%1"_s.arg(b.first), b.second);
            }
        }
    };
    template<typename T>
    auto generate_insert_helper(
        QStringView table_name, QStringView conflict, const QMetaObject& meta,
        std::span<const T> items, const std::set<std::string>& on_update,
        std::map<QString, std::function<QVariant(const QVariant&)>> converters = {},
        std::set<std::string_view>                                  ignores = {}) -> InsertHelper {
        InsertHelper             out;
        std::vector<std::string> column_names;
        std::set<std::string>    on_update_include, on_update_exclude;
        for (auto& el : on_update) {
            if (el.starts_with('^')) {
                on_update_exclude.insert(el.substr(1));
            } else {
                on_update_include.insert(el);
            }
        }

        for (int i = 0; i < meta.propertyCount(); i++) {
            auto name = meta.property(i).name();
            if (ignores.contains(name)) continue;
            column_names.push_back(name);
            out.binds.insert({ name, {} });
        }
        auto view_values =
            std::views::transform(column_names, [](const std::string& s) -> std::string {
                return ":" + s;
            });
        auto view_set = std::views::transform(
            std::views::filter(column_names,
                               [&on_update_include, &on_update_exclude](const std::string& s) {
                                   return (on_update_include.empty() ||
                                           on_update_include.contains(s)) &&
                                          ! on_update_exclude.contains(s);
                               }),
            [](const std::string& s) -> std::string {
                return fmt::format("{0} = :{0}", s);
            });
        out.sql = QString::fromStdString(
            fmt::format("INSERT INTO {0}({1}) VALUES ({2}) ON CONFLICT({3}) DO UPDATE SET {4};",
                        table_name,
                        fmt::join(column_names, ", "),
                        fmt::join(view_values, ", "),
                        conflict,
                        fmt::join(view_set, ", ")));
        for (int i = 0; i < meta.propertyCount(); i++) {
            auto name = meta.property(i).name();
            if (ignores.contains(name)) continue;
            auto& list = out.binds.at(name);

            std::optional<std::function<QVariant(const QVariant&)>> converter;

            if (converters.contains(name)) {
                converter = converters.at(name);
            }

            for (auto& el : items) {
                auto value = meta.property(i).readOnGadget(&el);
                if (converter) {
                    list << converter->operator()(value);
                } else if (auto converter = get_to_converter(value.metaType().id())) {
                    list << converter->operator()(value);
                } else {
                    list << value;
                }
            }
        }

        return out;
    }

private:
    void connect_db() {
        m_db   = QSqlDatabase::addDatabase("QSQLITE", m_name);
        auto p = m_db_path;
        m_db.setDatabaseName(p.native().c_str());

        if (m_db.open()) {
        } else {
            ERROR_LOG("{}", m_db.lastError().text());
        }
    }

    std::filesystem::path  m_db_path;
    QString                m_name;
    QThread                m_thread;
    rc<QtExecutionContext> m_ctx;
    QtExecutor             m_ex;
    QSqlDatabase           m_db;
};

} // namespace helper