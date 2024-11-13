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
#include "core/str_helper.h"
#include "core/qstr_helper.h"
#include "core/qmeta_helper.h"
#include "asio_qt/qt_executor.h"

namespace helper
{

struct SqlColumn {
    std::string name {};
    std::string type {};
    bool        notnull { false };
    std::string dflt_value {};
    bool        pk { false };

    std::strong_ordering operator<=>(const SqlColumn&) const = default;

    static auto from(QSqlQuery& query) {
        int       i = 1;
        SqlColumn c;
        c.name       = query.value(i++).toString().toStdString();
        c.type       = query.value(i++).toString().toStdString();
        c.notnull    = query.value(i++).toBool();
        c.dflt_value = query.value(i++).toString().toStdString();
        c.pk         = query.value(i++).toBool();
        return c;
    }
};
} // namespace helper

DEFINE_CONVERT(std::string, helper::SqlColumn) {
    out = fmt::format("{} {}{}{}{}",
                      in.name,
                      in.type,
                      in.notnull ? " NOT NULL"sv : ""sv,
                      in.dflt_value.empty() ? "" : fmt::format(" DEFAULT ({})", in.dflt_value),
                      in.pk ? " PRIMARY KEY"sv : ""sv);
}

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

        bool prepare_sv(std::string_view query) {
            return QSqlQuery::prepare(QString::fromUtf8(query));
        }

    private:
        QThread* m_thread;
    };

    using Converter = std::function<QVariant(const QVariant&)>;
    static auto get_from_converter(int id) -> std::optional<Converter>;
    static auto get_to_converter(int id) -> std::optional<Converter>;

    inline static auto EditTimeColumn = SqlColumn {
        .name       = "_editTime",
        .type       = "DATETIME",
        .dflt_value = "STRFTIME('%Y-%m-%dT%H:%M:%S.000Z', 'now')",
    };

    auto get_executor() -> QtExecutor& { return m_ex; }

    auto is_open() const -> bool { return m_db.isOpen(); }

    auto query() const -> Query { return Query(m_db); }
    auto db() -> QSqlDatabase& { return m_db; }
    auto error_str() -> QString { return m_db.lastError().text(); }

    auto generate_column_migration(QStringView table_name, std::span<const SqlColumn> req_columns,
                                   std::span<const std::string> extra = {}) -> QStringList {
        QSqlQuery                query(m_db);
        QString                  pragmaQuery = QString("PRAGMA table_info(%1);").arg(table_name);
        std::vector<std::string> columns;
        std::ranges::copy(std::views::transform(req_columns,
                                                [](const auto& in) {
                                                    return convert_from<std::string>(in);
                                                }),
                          std::back_inserter(columns));
        columns.insert(columns.end(), extra.begin(), extra.end());

        QString create_sql = QString::fromStdString(fmt::format(R"(
CREATE TABLE IF NOT EXISTS {} (
{}
);
)",
                                                                table_name,
                                                                fmt::join(columns, ",\n")));

        std::set<SqlColumn, std::less<>> current_columns;

        if (query.exec(pragmaQuery)) {
            while (query.next()) {
                current_columns.insert(SqlColumn::from(query));
            }
        }

        if (current_columns.size() > 0) {
            std::set<std::string> common_columns;
            for (auto& el : req_columns) {
                if (current_columns.contains(el)) {
                    common_columns.insert(el.name);
                }
            }
            if (common_columns.size() < req_columns.size()) {
                auto column_names = QString::fromStdString(fmt::format(
                    "{}",
                    fmt::join(std::views::transform(common_columns,
                                                    [](std::string_view in) {
                                                        return in.substr(0, in.find_first_of(' '));
                                                    }),
                              ", ")));

                QStringList out;
                out << u"ALTER TABLE %1 RENAME TO %1_backup;"_s.arg(table_name);
                out << create_sql;
                out << u"INSERT INTO %1 (%2) SELECT %2 FROM %1_backup;"_s.arg(table_name,
                                                                              column_names);
                out << u"DROP TABLE %1_backup;"_s.arg(table_name);
                return out;
            }
        }
        return { create_sql };
    }

    auto generate_meta_migration(QStringView table_name, QStringView primary,
                                 const QMetaObject& meta, std::span<const std::string> extras,
                                 std::set<std::string_view> exclude = {}) {
        std::vector<std::string> column_names;
        std::vector<SqlColumn>   columns;
        for (int i = 0; i < meta.propertyCount(); i++) {
            const auto& p    = meta.property(i);
            auto        name = p.name();
            if (exclude.contains(name)) {
                continue;
            }

            auto& c = columns.emplace_back();

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
            c.type = type;
            c.name = name;
            c.pk   = name == primary;
        }

        columns.emplace_back(EditTimeColumn);
        return generate_column_migration(table_name, columns, extras);
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

    // insert with columns
    template<typename T>
    auto generate_insert_helper(QStringView                               table_name,
                                const std::set<std::string, std::less<>>& conflicts,
                                const QMetaObject& meta, std::span<const T> items,
                                const std::set<std::string>&        columns,
                                const std::set<std::string>&        on_update  = {},
                                const std::map<QString, Converter>& converters = {},
                                bool use_edit_time = true) const -> InsertHelper {
        InsertHelper out = generate_insert_helper_sql(
            table_name, conflicts, meta, columns, on_update, use_edit_time);

        for (int i = 0; i < meta.propertyCount(); i++) {
            auto name = meta.property(i).name();
            if (! out.binds.contains(name)) continue;
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
    auto generate_insert_helper_sql(QStringView                               table_name,
                                    const std::set<std::string, std::less<>>& conflicts,
                                    const QMetaObject& meta, const std::set<std::string>& columns,
                                    const std::set<std::string>& on_update,
                                    bool use_edit_time) const -> InsertHelper {
        InsertHelper               out;
        std::vector<std::string>   column_names;
        std::set<std::string_view> column_include, column_exclude, on_update_include,
            on_update_exclude;

        auto column_filter = [&conflicts, &column_include, &column_exclude](std::string_view in) {
            return (column_include.empty() || column_include.contains(in) ||
                    conflicts.contains(in)) &&
                   ! column_exclude.contains(in);
        };
        auto on_update_filter =
            [&conflicts, &on_update_include, &on_update_exclude](std::string_view in) {
                return (on_update_include.empty() || on_update_include.contains(in)) &&
                       ! conflicts.contains(in) && ! on_update_exclude.contains(in);
            };
        {
            for (auto& s : columns) {
                auto el = std::string_view(s);
                if (el.starts_with('^')) {
                    column_exclude.insert(el.substr(1));
                } else {
                    column_include.insert(el);
                }
            }
            for (auto& s : on_update) {
                auto el = std::string_view(s);
                if (el.starts_with('^')) {
                    on_update_exclude.insert(el.substr(1));
                } else {
                    on_update_include.insert(el);
                }
            }
        }

        for (int i = 0; i < meta.propertyCount(); i++) {
            auto name = meta.property(i).name();
            if (! column_filter(name)) continue;
            column_names.push_back(name);
            out.binds.insert({ name, {} });
        }
        auto view_values =
            std::views::transform(column_names, [](const std::string& s) -> std::string {
                return ":" + s;
            });
        auto view_set = std::views::transform(std::views::filter(column_names, on_update_filter),
                                              [](const std::string& s) -> std::string {
                                                  return fmt::format("{0} = :{0}", s);
                                              });
        out.sql       = QString::fromStdString(fmt::format(
            "INSERT INTO {0}({1}) VALUES ({2}) ON CONFLICT({3}) DO UPDATE SET {4}{5};",
            table_name,
            fmt::join(column_names, ", "),
            fmt::join(view_values, ", "),
            fmt::join(conflicts, ", "),
            fmt::join(view_set, ", "),
            use_edit_time ? ", _editTime = (STRFTIME('%Y-%m-%dT%H:%M:%S.000Z', 'now'))"s : ""s));
        return out;
    }

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