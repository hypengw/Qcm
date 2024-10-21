#include "Qcm/sql/collection_sql.h"

#include <QSqlError>
#include <QSqlQuery>
#include <asio/dispatch.hpp>
#include <asio/bind_executor.hpp>
#include <asio/use_awaitable.hpp>

#include "qcm_interface/path.h"
#include "core/str_helper.h"
#include "core/qstr_helper.h"
#include "core/log.h"
#include "asio_qt/qt_sql.h"

namespace qcm
{

CollectionSql::CollectionSql(std::string_view table, rc<helper::SqlConnect> con)
    : m_table(convert_from<QString>(table)), m_con(con) {
    asio::dispatch(con->get_executor(), [this]() {
        connect_db();
    });
}
CollectionSql::~CollectionSql() {}

auto CollectionSql::get_executor() -> QtExecutor& { return m_con->get_executor(); }

void CollectionSql::connect_db() {
    if (m_con->is_open()) {
        auto    q           = m_con->query();
        QString createTable = QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        userId TEXT NOT NULL,
        type TEXT NOT NULL,
        itemId TEXT NOT NULL,
        collectTime DATETIME DEFAULT CURRENT_TIMESTAMP,
        UNIQUE(userId, itemId)
    )
)")
                                  .arg(m_table);

        auto migs = m_con->generate_column_migration(
            m_table,
            createTable,
            std::array { "id"s, "userId"s, "type"s, "itemId"s, "collectTime"s });

        for (auto& el : migs) {
            if (! q.exec(el)) {
                ERROR_LOG("{}", q.lastError().text());
            }
        }
    }
}

auto CollectionSql::insert(std::span<const Item> items) -> asio::awaitable<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    co_return insert_sync(items);
}
auto CollectionSql::remove(model::ItemId userId, model::ItemId itemId) -> asio::awaitable<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    co_return remove_sync(userId, itemId);
}
auto CollectionSql::select_id(model::ItemId userId,
                              QString) -> asio::awaitable<std::vector<model::ItemId>> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    query.prepare(QStringLiteral("SELECT itemId FROM %1 WHERE userId = :userId").arg(m_table));
    query.bindValue(":userId", userId.toUrl());

    std::vector<model::ItemId> out;
    if (query.exec()) {
        while (query.next()) {
            out.emplace_back(model::ItemId(query.value("itemId").toString()));
        }
    } else {
        ERROR_LOG("{}", query.lastError().text());
    }
    co_return out;
}

auto CollectionSql::refresh(model::ItemId userId, QString type,
                            std::span<const model::ItemId> itemIds,
                            std::span<const QDateTime>     times) -> asio::awaitable<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    if (! m_con->db().transaction()) {
        ERROR_LOG("{}", m_con->error_str());
        co_return false;
    }

    if (! un_valid(userId, type) || ! insert_sync(userId, itemIds, times) || ! clean_not_valid()) {
        m_con->db().rollback();
        co_return false;
    }

    if (! m_con->db().commit()) {
        ERROR_LOG("{}", m_con->error_str());
        co_return false;
    }

    co_return true;
}

bool CollectionSql::insert_sync(std::span<const Item> items) {
    auto query = m_con->query();
    query.prepare(QStringLiteral(R"(
    INSERT INTO %1 (userId, type, itemId, collectTime)
    VALUES (:userId, :type, :itemId, :collectTime)
)")
                      .arg(m_table));

    for (auto& item : items) {
        query.bindValue(":userId", item.user_id.toUrl());
        query.bindValue(":type", item.type);
        query.bindValue(":itemId", item.item_id.toUrl());
        query.bindValue(":collectTime", item.collect_time.value_or(QDateTime::currentDateTime()));

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
    query.prepare(QStringLiteral(R"(
INSERT INTO %1 (userId, itemId, type, collectTime)
VALUES (:userId, :itemId, :type, :collectTime)
ON CONFLICT(userId, itemId) DO UPDATE
SET 
type = :type,
collectTime = :collectTime;
)")
                      .arg(m_table));
    auto cur = QDateTime::currentDateTime();
    for (usize i = 0; i < ids.size(); i++) {
        query.bindValue(":userId", userId.toUrl());
        query.bindValue(":itemId", ids[i].toUrl());
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

bool CollectionSql::remove_sync(model::ItemId userId, model::ItemId itemId) {
    auto query = m_con->query();
    query.prepare(QStringLiteral(R"(DELETE FROM %1 WHERE userId = :userId AND itemId = :itemId)")
                      .arg(m_table));

    query.bindValue(":userId", userId.toUrl());
    query.bindValue(":itemId", itemId.toUrl());

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        return false;
    }
    return true;
}

bool CollectionSql::delete_with(model::ItemId userId, QString type) {
    auto query = m_con->query();

    QStringList cond;
    cond << "userId = :userId";
    if (! type.isEmpty()) cond << "type = :type";
    query.prepare(QStringLiteral("DELETE FROM %1 WHERE %2").arg(m_table).arg(cond.join(" AND ")));
    query.bindValue(":userId", userId.toUrl());
    if (! type.isEmpty()) query.bindValue(":type", type);

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        return false;
    }
    return true;
}

bool CollectionSql::un_valid(model::ItemId userId, QString type) {
    auto query = m_con->query();

    QStringList cond;
    cond << "userId = :userId";
    if (! type.isEmpty()) cond << "type = :type";

    query.prepare(QStringLiteral(R"(
UPDATE %1
SET type = 'invalid'
WHERE %2
)")
                      .arg(m_table)
                      .arg(cond.join(" AND ")));

    query.bindValue(":userId", userId.toUrl());
    if (! type.isEmpty()) query.bindValue(":type", type);

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        return false;
    }
    return true;
}

bool CollectionSql::clean_not_valid() {
    auto query = m_con->query();
    query.prepare(QStringLiteral(R"(DELETE FROM %1 WHERE type = 'invalid')").arg(m_table));
    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        return false;
    }
    return true;
}

} // namespace qcm