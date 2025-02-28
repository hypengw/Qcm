#include "Qcm/sql/collection_sql.h"

#include <QSqlError>
#include <QSqlQuery>
#include <asio/dispatch.hpp>
#include <asio/bind_executor.hpp>
#include <asio/use_awaitable.hpp>

#include "platform/platform.h"
#include "qcm_interface/sql/meta_sql.h"
#include "qcm_interface/path.h"
#include "core/str_helper.h"
#include "core/qstr_helper.h"
#include "core/log.h"
#include "asio_qt/qt_sql.h"
#include "qcm_interface/global.h"

namespace qcm
{

CollectionSql::CollectionSql(std::string_view table, rc<helper::SqlConnect> con)
    : m_table(convert_from<QString>(table)), m_con(con) {
    asio::dispatch(con->get_executor(), [this]() {
        plt::set_thread_name("sql_collection");
        connect_db();
    });
}
CollectionSql::~CollectionSql() {}

auto CollectionSql::get_executor() -> QtExecutor& { return m_con->get_executor(); }
auto CollectionSql::con() const -> rc<helper::SqlConnect> { return m_con; }

void CollectionSql::connect_db() {
    if (m_con->is_open()) {
        auto q    = m_con->query();
        auto migs = m_con->generate_column_migration(
            m_table,
            std::array {
                helper::SqlColumn { .name    = "libraryId",
                                    .type    = "INTEGER",
                                    .notnull = 1,
                                    .foreign = helper::SqlForeignKey { .from_ = "libraryId",
                                                                       .table = "library",
                                                                       .to    = "libraryId" } },
                helper::SqlColumn { .name = "userId", .type = "TEXT", .notnull = 1 },
                helper::SqlColumn { .name = "type", .type = "TEXT", .notnull = 1 },
                helper::SqlColumn { .name = "itemId", .type = "TEXT", .notnull = 1 },
                helper::SqlColumn { .name       = "collectTime",
                                    .type       = "DATETIME",
                                    .dflt_value = "STRFTIME('%Y-%m-%dT%H:%M:%S.000Z', 'now')" },
                helper::SqlColumn { .name = "removed", .type = "INTEGER", .dflt_value = "0" },
            },
            { { "userId"sv, "itemId"sv } });

        for (auto& el : migs) {
            if (! q.exec(el)) {
                ERROR_LOG("{}", q.lastError().text());
            }
        }
    }
}

auto CollectionSql::insert(std::span<const Item> items) -> task<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    co_return insert_sync(items);
}
auto CollectionSql::remove(model::ItemId userId, model::ItemId itemId) -> task<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    co_return remove_sync(userId, std::array { itemId });
}

auto CollectionSql::remove(model::ItemId user_id, std::span<const model::ItemId> ids)
    -> task<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    co_return remove_sync(user_id, ids);
}

auto CollectionSql::select_id(model::ItemId userId, QString type)
    -> task<std::vector<model::ItemId>> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto    query = m_con->query();
    QString cond;
    if (! type.isEmpty()) {
        cond = "AND type = :type";
    }
    query.prepare("SELECT itemId,type,libraryId FROM {} WHERE userId = :userId {}", m_table, cond);
    query.bindValue(":userId", userId.id());
    if (! type.isEmpty()) {
        query.bindValue(":type", type);
    }

    std::vector<model::ItemId> out;
    if (query.exec()) {
        while (query.next()) {
            model::ItemId id;
            id.set_id(query.value("itemId").toString());
            id.set_type(query.value("type").toString());
            id.set_library_id(query.value("libraryId").toLongLong());
            out.push_back(id);
        }
    } else {
        ERROR_LOG("{}", query.lastError().text());
    }
    co_return out;
}
auto CollectionSql::select_removed(model::ItemId user_id, const QString& type,
                                   const QDateTime& time) -> task<std::vector<model::ItemId>> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    query.prepare(
        R"(
SELECT 
    itemId,
    libraryId 
FROM {} 
WHERE userId = :userId AND type = :type AND collectTime >= :time AND removed = 1;
)",
        m_table);
    query.bindValue(":userId", user_id.id());
    query.bindValue(":type", type);
    query.bindValue(":time", time.toUTC().addSecs(-1));

    std::vector<model::ItemId> out;
    if (query.exec()) {
        while (query.next()) {
            auto id = model::ItemId(query.value(0).toString());
            id.set_library_id(query.value(1).toLongLong());
            out.emplace_back(id);
        }
    } else {
        ERROR_LOG("{}", query.lastError().text());
    }
    co_return out;
}

auto CollectionSql::select_missing(const model::ItemId& user_id, std::string_view type,
                                   std::string_view join, const std::set<std::string>& not_null)
    -> task<std::vector<model::ItemId>> {
    std::vector<model::ItemId> ids;
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = con()->query();
    query.prepare(
        R"(
SELECT 
    collection.itemId,
    collection.libraryId
FROM collection
LEFT JOIN {1} ON {1}.itemId = collection.itemId  
WHERE collection.userId = :userId AND collection.type = '{0}' AND collection.removed = 0 AND ({2});
)",
        type,
        join,
        db::null<db::OR, db::EQ>(not_null, join));
    query.bindValue(":userId", user_id.id());

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
    }
    auto x = query.lastQuery();
    while (query.next()) {
        model::ItemId id;
        id.set_id(query.value(0).toString());
        id.set_library_id(query.value(1).toLongLong());
        ids.emplace_back(id);
    }
    co_return ids;
}

auto CollectionSql::refresh(model::ItemId user_id, i64 provider_id, QString type,
                            std::span<const model::ItemId> itemIds,
                            std::span<const QDateTime>     times) -> task<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    if (! m_con->transaction()) {
        co_return false;
    }

    if (! remove_sync(user_id, provider_id, type) || ! insert_sync(user_id, itemIds, times) ||
        ! delete_removed()) {
        m_con->rollback();
        co_return false;
    }

    if (! m_con->commit()) {
        co_return false;
    }

    co_return true;
}

bool CollectionSql::insert_sync(std::span<const Item> items) {
    auto query = m_con->query();
    query.prepare(R"(
INSERT INTO {0} (libraryId, userId, type, itemId, collectTime)
VALUES (:libraryId, :userId, :type, :itemId, :collectTime)
ON CONFLICT(libraryId, userId, itemId) DO UPDATE
SET 
type = :type,
collectTime = :collectTime,
removed = 0;
)",
                  m_table);

    auto library_id = QVariant::fromValue((i64)0);
    for (auto& item : items) {
        library_id.setValue(item.item_id.library_id());
        query.bindValue(":libraryId", library_id);
        query.bindValue(":userId", item.user_id.id());
        query.bindValue(":type", item.type);
        query.bindValue(":itemId", item.item_id.id());
        query.bindValue(":collectTime",
                        item.collect_time.value_or(QDateTime::currentDateTimeUtc()));

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
            return false;
        }
    }
    return true;
}

bool CollectionSql::insert_sync(model::ItemId userId, std::span<const model::ItemId> ids,
                                std::span<const QDateTime> times) {
    auto query = m_con->query();
    query.prepare(R"(
INSERT INTO {} (libraryId, userId, itemId, type, collectTime)
VALUES (:libraryId, :userId, :itemId, :type, :collectTime)
ON CONFLICT(userId, itemId) DO UPDATE
SET 
type = :type,
collectTime = :collectTime,
removed = 0;
)",
                  m_table);
    auto cur         = QDateTime::currentDateTimeUtc();
    auto library_id  = QVariant::fromValue((i64)0);
    auto provider_id = QVariant::fromValue((i64)0);

    for (usize i = 0; i < ids.size(); i++) {
        library_id.setValue(ids[i].library_id());
        query.bindValue(":libraryId", library_id);
        query.bindValue(":providerId", provider_id);
        query.bindValue(":userId", userId.id());
        query.bindValue(":itemId", ids[i].id());
        query.bindValue(":type", ids[i].type());
        if (i < times.size()) {
            query.bindValue(":collectTime", times[i]);
        } else {
            query.bindValue(":collectTime", cur);
        }

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
            return false;
        }
    }
    return true;
}

bool CollectionSql::remove_sync(model::ItemId user_id, std::span<const model::ItemId> ids) {
    if (ids.empty()) return true;
    QStringList list;
    for (usize i = 0; i < ids.size(); i++) {
        list << QString(":id%1").arg(i);
    }
    auto query = m_con->query();
    query.prepare(R"(
UPDATE {0}
SET removed = 1, collectTime = (STRFTIME('%Y-%m-%dT%H:%M:%S.000Z', 'now'))
WHERE libraryId = :libraryId AND userId = :userId AND itemId IN ({1});
)",
                  m_table,
                  list.join(","));

    auto library_id = QVariant::fromValue(ids[0].library_id());
    query.bindValue(":libraryId", library_id);
    query.bindValue(":userId", user_id.id());

    for (usize i = 0; i < ids.size(); i++) {
        query.bindValue(list[i], ids[i].id());
    }

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        return false;
    }
    return true;
}

bool CollectionSql::delete_with(model::ItemId userId, i64 provider_id, QString type) {
    auto query = m_con->query();

    QStringList cond;
    cond << "userId = :userId";
    if (! type.isEmpty()) cond << "type = :type";
    query.prepare(R"(
DELETE FROM {0}
JOIN library ON library.providerId = {2}
WHERE {1} AND library.libraryId = {0}.libraryId
)",
                  m_table,
                  cond.join(" AND "),
                  provider_id);
    query.bindValue(":userId", userId.id());
    if (! type.isEmpty()) query.bindValue(":type", type);

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        return false;
    }
    return true;
}

bool CollectionSql::remove_sync(model::ItemId userId, i64 provider_id, QString type) {
    auto query = m_con->query();

    QStringList cond;
    cond << "userId = :userId";
    if (! type.isEmpty()) cond << "type = :type";

    query.prepare(R"(
UPDATE {0}
SET removed = 1
WHERE {1} AND libraryId IN (SELECT libraryId FROM library WHERE providerId = {2})
)",
                  m_table,
                  cond.join(" AND "),
                  provider_id);

    query.bindValue(":userId", userId.id());
    if (! type.isEmpty()) query.bindValue(":type", type);

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        return false;
    }
    return true;
}

bool CollectionSql::delete_removed() {
    auto query = m_con->query();
    query.prepare(R"(DELETE FROM {} WHERE removed = 1)", m_table);
    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        return false;
    }
    return true;
}

} // namespace qcm