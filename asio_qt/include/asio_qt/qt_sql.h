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

class SqlQuery : public QSqlQuery {
public:
    SqlQuery(const QSqlDatabase& db): QSqlQuery(db), m_thread(db.thread()) {
        _assert_rel_(m_thread == nullptr || m_thread->isCurrentThread());
    }
    ~SqlQuery() { _assert_rel_(m_thread == nullptr || m_thread->isCurrentThread()); }
    SqlQuery(const SqlQuery&)            = delete;
    SqlQuery& operator=(const SqlQuery&) = delete;
    SqlQuery(SqlQuery&& o): QSqlQuery(std::move(o)), m_thread(std::exchange(o.m_thread, nullptr)) {}
    SqlQuery& operator=(SqlQuery&& o) {
        QSqlQuery::operator=(std::move(o));
        m_thread = std::exchange(o.m_thread, nullptr);
        return *this;
    }

    bool prepare_sv(std::string_view query) { return QSqlQuery::prepare(QString::fromUtf8(query)); }
    bool exec(const QString&             sql,
              const std::source_location loc = std::source_location::current()) {
        if (! QSqlQuery::exec(sql)) {
            qcm::log::log(qcm::LogLevel::ERROR, loc, "{}", lastError().text());
            return false;
        }
        return true;
    }
    bool exec(const std::source_location loc = std::source_location::current()) {
        if (! QSqlQuery::exec()) {
            qcm::log::log(qcm::LogLevel::ERROR, loc, "{}", lastError().text());
            return false;
        }
        return true;
    }

private:
    QThread* m_thread;
};

struct SqlColumn {
    std::string name {};
    std::string type {};
    bool        notnull { false };
    std::string dflt_value {};
    bool        pk { false };

    std::strong_ordering operator<=>(const SqlColumn&) const = default;

    static auto from(QSqlQuery& query) {
        SqlColumn c;
        c.name       = query.value("name").toString().toStdString();
        c.type       = query.value("type").toString().toStdString();
        c.notnull    = query.value("notnull").toBool();
        c.dflt_value = query.value("dflt_value").toString().toStdString();
        c.pk         = query.value("pk").toBool();
        return c;
    }

    static auto query(SqlQuery& q, QAnyStringView name) -> std::vector<SqlColumn> {
        std::vector<SqlColumn> columns;
        q.prepare_sv(fmt::format("PRAGMA table_info({});", name));
        if (q.exec()) {
            while (q.next()) {
                columns.emplace_back(SqlColumn::from(q));
            }
        }
        return columns;
    }
};

struct SqlIndex {
    i64         seq { 0 };
    std::string name {};
    bool        unique { false };
    // "u" or "pk"
    std::string origin {};
    bool        partial {};

    struct Info {
        i64         seqno { 0 };
        i64         cid { 0 };
        std::string name {};

        std::strong_ordering operator<=>(const Info&) const = default;

        static auto from(QSqlQuery& query) {
            Info c;
            c.seqno = query.value("seqno").toInt();
            c.cid   = query.value("cid").toInt();
            c.name  = query.value("name").toString().toStdString();
            return c;
        }

        static auto query(SqlQuery& q, QAnyStringView name) -> std::vector<Info> {
            std::vector<Info> out;
            q.prepare_sv(fmt::format("PRAGMA index_info({});", name));
            if (q.exec()) {
                while (q.next()) {
                    out.emplace_back(Info::from(q));
                }
            }
            return out;
        }
    };
    std::vector<Info> infos;

    std::strong_ordering operator<=>(const SqlIndex&) const = default;

    static auto from(QSqlQuery& query) {
        SqlIndex c;
        c.seq     = query.value("seq").toInt();
        c.name    = query.value("name").toString().toStdString();
        c.unique  = query.value("unique").toBool();
        c.origin  = query.value("origin").toString().toStdString();
        c.partial = query.value("partial").toBool();
        return c;
    }

    static auto query(SqlQuery& q, QAnyStringView name) -> std::vector<SqlIndex> {
        std::vector<SqlIndex> out;
        q.prepare_sv(fmt::format("PRAGMA index_list({});", name));
        if (q.exec()) {
            while (q.next()) {
                out.emplace_back(SqlIndex::from(q));
            }
        }

        for (auto& el : out) {
            el.infos = Info::query(q, el.name);
        }
        return out;
    }
};

struct SqlUnique {
    SqlUnique(std::initializer_list<std::string_view> strs) {
        columns = fmt::format("{}", fmt::join(strs, ","));
        single  = strs.size() == 1;
    }
    template<std::ranges::range R>
        requires std::same_as<SqlIndex::Info, std::ranges::range_value_t<R>>
    SqlUnique(const R& r) {
        columns = fmt::format("{}",
                              fmt::join(std::views::transform(r,
                                                              [](const auto& el) {
                                                                  return el.name;
                                                              }),
                                        ","));
        single  = columns.find_first_of(',') == std::string::npos;
    }

    std::string columns {};
    bool        single { false };

    std::strong_ordering operator<=>(const SqlUnique&) const = default;
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

    auto transaction(const std::source_location loc = std::source_location::current()) -> bool {
        auto ok = db().transaction();
        if (! ok) {
            qcm::log::log(qcm::LogLevel::ERROR, loc, "{}", db().lastError().text());
        }
        return ok;
    }

    auto rollback(const std::source_location loc = std::source_location::current()) -> bool {
        auto ok = db().rollback();
        if (! ok) {
            qcm::log::log(qcm::LogLevel::ERROR, loc, "{}", db().lastError().text());
        }
        return ok;
    }

    auto commit(const std::source_location loc = std::source_location::current()) -> bool {
        auto ok = db().commit();
        if (! ok) {
            qcm::log::log(qcm::LogLevel::ERROR, loc, "{}", db().lastError().text());
        }
        return ok;
    }

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

    auto query() const -> SqlQuery { return SqlQuery(m_db); }
    auto db() -> QSqlDatabase& { return m_db; }
    auto error_str() -> QString { return m_db.lastError().text(); }

    auto check_fts_changed(QStringView                               table_name,
                           const std::set<std::string, std::less<>>& columns) -> bool {
        SqlQuery query(m_db);
        auto     current_columns = SqlColumn::query(query, QString("%1_fts").arg(table_name));
        if (current_columns.size() != columns.size()) {
            return true;
        }
        for (auto& el : current_columns) {
            if (! columns.contains(el.name)) {
                return true;
            }
        }
        return false;
    }

    auto drop_fts_trigger(QStringView table_name) -> QStringList {
        QStringList out;
        out << u"DROP TRIGGER IF EXISTS %1_fts_i;"_s.arg(table_name);
        out << u"DROP TRIGGER IF EXISTS %1_fts_d;"_s.arg(table_name);
        out << u"DROP TRIGGER IF EXISTS %1_fts_u;"_s.arg(table_name);
        return out;
    }

    auto fts_trigger(QStringView table_name, const std::set<std::string, std::less<>>& columns,
                     QStringView id_column = u"rowid") -> QStringList {
        QStringList out;
        auto        column_list = fmt::format("{}", fmt::join(columns, ", "));
        auto        new_prefix_column_list =
            fmt::format("{}",
                        fmt::join(std::views::transform(columns,
                                                        [](const auto& el) {
                                                            return fmt::format("new.{}", el);
                                                        }),
                                  ", "));
        auto old_prefix_column_list =
            fmt::format("{}",
                        fmt::join(std::views::transform(columns,
                                                        [](const auto& el) {
                                                            return fmt::format("old.{}", el);
                                                        }),
                                  ", "));

        out << QString::fromStdString(fmt::format(
            R"(
CREATE TRIGGER {0}_fts_i AFTER INSERT ON {0}
BEGIN
    INSERT INTO {0}_fts(rowid, {2}) VALUES (new.{1}, {3});
END;
)",
            table_name,
            id_column,
            column_list,
            new_prefix_column_list));

        out << QString::fromStdString(fmt::format(
            R"(
CREATE TRIGGER {0}_fts_d AFTER DELETE ON {0}
BEGIN
    INSERT INTO {0}_fts({0}_fts, rowid, {2}) VALUES('delete', old.{1}, {3});
END;
)",
            table_name,
            id_column,
            column_list,
            old_prefix_column_list));

        out << QString::fromStdString(fmt::format(
            R"(
CREATE TRIGGER {0}_fts_u AFTER UPDATE ON {0}
BEGIN
    INSERT INTO {0}_fts({0}_fts, rowid, {2}) VALUES('delete', old.{1}, {3});
    INSERT INTO {0}_fts(rowid, {2}) VALUES (new.{1}, {3});
END;
)",
            table_name,
            id_column,
            column_list,
            old_prefix_column_list,
            new_prefix_column_list));

        return out;
    }

    auto create_fts(QStringView table_name, bool drop,
                    const std::set<std::string, std::less<>>& fts_columns,
                    QStringView                               id_column = u"rowid") -> QStringList {
        QStringList out;
        if (drop) {
            out << u"DROP TABLE IF EXISTS %1_fts;"_s.arg(table_name);
        }
        out << drop_fts_trigger(table_name);
        out << QString::fromStdString(fmt::format(R"(
CREATE VIRTUAL TABLE IF NOT EXISTS {0}_fts USING fts5 (
{2},
content='{0}',
content_rowid='{1}'
);
)",
                                                  table_name,
                                                  id_column,
                                                  fmt::join(fts_columns, ", ")));

        if (drop) {
            out << QString::fromStdString(
                fmt::format("INSERT INTO {0}_fts({1}) SELECT {1} FROM {0};",
                            table_name,
                            fmt::join(fts_columns, ", ")));
        }
        out << fts_trigger(table_name, fts_columns);
        return out;
    }

    auto generate_column_migration(QStringView table_name, std::span<const SqlColumn> req_columns,
                                   const std::set<SqlUnique, std::less<>>&   uniques     = {},
                                   const std::set<std::string, std::less<>>& fts_columns = {})
        -> QStringList {
        SqlQuery                 query(m_db);
        std::vector<std::string> columns;
        std::ranges::copy(std::views::transform(req_columns,
                                                [&uniques](const auto& in) {
                                                    std::string out = convert_from<std::string>(in);
                                                    if (! in.pk && uniques.contains({ in.name })) {
                                                        out.append(" UNIQUE");
                                                    }
                                                    return out;
                                                }),
                          std::back_inserter(columns));

        // append like UNIQUE(C1,c2)
        std::ranges::copy(std::views::transform(std::views::filter(uniques,
                                                                   [](const auto& el) {
                                                                       return ! el.single;
                                                                   }),
                                                [](const auto& el) {
                                                    return fmt::format("UNIQUE({})", el.columns);
                                                }),
                          std::back_inserter(columns));

        QString create_sql = QString::fromStdString(fmt::format(R"(
CREATE TABLE IF NOT EXISTS {} (
{}
);
)",
                                                                table_name,
                                                                fmt::join(columns, ",\n")));

        std::set<SqlColumn, std::less<>> current_columns;
        std::set<SqlUnique, std::less<>> current_uniques;
        std::ranges::copy(SqlColumn::query(query, table_name),
                          std::inserter(current_columns, current_columns.end()));
        std::ranges::for_each(SqlIndex::query(query, table_name),
                              [&current_uniques](const SqlIndex& el) {
                                  current_uniques.insert({ el.infos });
                              });

        if (current_columns.size() > 0) {
            auto find_pk = [](const auto& vec) -> std::optional<std::string> {
                if (auto it = std::find_if(vec.begin(),
                                           vec.end(),
                                           [](const auto& el) {
                                               return el.pk;
                                           });
                    it != vec.end()) {
                    return it->name;
                }
                return std::nullopt;
            };

            std::set<std::string>            common_columns;
            std::map<std::string, SqlColumn> current_column_map;
            for (auto& el : current_columns) current_column_map.insert({ el.name, el });

            for (auto& el : req_columns) {
                if (auto it = current_column_map.find(el.name);
                    it != current_column_map.end() && it->second.type == el.type) {
                    common_columns.insert(el.name);
                }
            }
            // check columns, uniques and primary key
            if (common_columns.size() < req_columns.size() || uniques != current_uniques ||
                find_pk(current_columns) != find_pk(req_columns)) {
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
                if (! fts_columns.empty()) {
                    out << create_fts(table_name, true, fts_columns);
                }
                return out;
            }
        }
        QStringList out { create_sql };
        if (! fts_columns.empty()) {
            bool changed = check_fts_changed(table_name, fts_columns);
            out << create_fts(table_name, changed, fts_columns);
        }
        return out;
    }

    auto generate_meta_migration(QStringView table_name, QStringView primary,
                                 const QMetaObject&                        meta,
                                 const std::set<SqlUnique, std::less<>>&   uniques,
                                 const std::set<std::string, std::less<>>& fts_columns = {},
                                 const std::set<std::string_view>& exclude = {}) -> QStringList {
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
        return generate_column_migration(table_name, columns, uniques, fts_columns);
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