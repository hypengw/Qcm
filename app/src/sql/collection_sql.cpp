#include "Qcm/sql/collection_sql.h"

#include <QSqlError>
#include <QSqlQuery>
#include <asio/dispatch.hpp>
#include <asio/bind_executor.hpp>
#include <asio/use_awaitable.hpp>

#include "qcm_interface/path.h"
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
        user_id TEXT NOT NULL,
        type TEXT NOT NULL,
        item_id TEXT NOT NULL,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
        UNIQUE(user_id, item_id)
    )
)")
                                  .arg(m_table);
        if (! q.exec(createTable)) {
            ERROR_LOG("creating table '{}' failed: {}", m_table, q.lastError().text());
        }
    }
}

auto CollectionSql::insert(std::span<const Item> items) -> asio::awaitable<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    co_return insert_sync(items);
}
auto CollectionSql::remove(model::ItemId user_id, model::ItemId item_id) -> asio::awaitable<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    co_return remove_sync(user_id, item_id);
}
auto CollectionSql::select_id(model::ItemId user_id,
                              QString) -> asio::awaitable<std::vector<model::ItemId>> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    query.prepare(QStringLiteral("SELECT item_id FROM %1 WHERE user_id = :userId").arg(m_table));
    query.bindValue(":userId", user_id.toUrl());

    std::vector<model::ItemId> out;
    if (query.exec()) {
        while (query.next()) {
            out.emplace_back(model::ItemId(query.value("item_id").toString()));
        }
    } else {
        ERROR_LOG("{}", query.lastError().text());
    }
    co_return out;
}

auto CollectionSql::refresh(model::ItemId user_id, QString type,
                            std::span<const model::ItemId> item_ids) -> asio::awaitable<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    if (! m_con->db().transaction()) {
        ERROR_LOG("{}", m_con->error_str());
        co_return false;
    }

    if (! un_valid(user_id, type) || ! insert_sync(user_id, item_ids) || ! clean_not_valid()) {
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
    INSERT INTO %1 (user_id, type, item_id, created_at)
    VALUES (:user_id, :type, :item_id, :created_at)
)")
                      .arg(m_table));

    for (auto& item : items) {
        query.bindValue(":user_id", item.user_id.toUrl());
        query.bindValue(":type", item.type);
        query.bindValue(":item_id", item.item_id.toUrl());
        query.bindValue(":created_at", item.created_at.value_or(QDateTime::currentDateTime()));

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
            return false;
        }
    }
    return true;
}

bool CollectionSql::insert_sync(model::ItemId userId, std::span<const model::ItemId> ids) {
    auto query = m_con->query();
    query.prepare(QStringLiteral(R"(
INSERT INTO %1 (user_id, item_id, type)
VALUES (:user_id, :item_id, :type)
ON CONFLICT(user_id, item_id) DO UPDATE
SET 
type = :type;
)")
                      .arg(m_table));
    for (auto& id : ids) {
        query.bindValue(":user_id", userId.toUrl());
        query.bindValue(":item_id", id.toUrl());
        query.bindValue(":type", id.type());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
            return false;
        }
    }
    return true;
}

bool CollectionSql::remove_sync(model::ItemId user_id, model::ItemId item_id) {
    auto query = m_con->query();
    query.prepare(
        QStringLiteral(R"(DELETE FROM %1 WHERE user_id = :user_id AND item_id = :item_id)")
            .arg(m_table));

    query.bindValue(":user_id", user_id.toUrl());
    query.bindValue(":item_id", item_id.toUrl());

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        return false;
    }
    return true;
}

bool CollectionSql::delete_with(model::ItemId user_id, QString type) {
    auto query = m_con->query();

    QStringList cond;
    cond << "user_id = :user_id";
    if (! type.isEmpty()) cond << "type = :type";
    query.prepare(QStringLiteral("DELETE FROM %1 WHERE %2").arg(m_table).arg(cond.join(" AND ")));
    query.bindValue(":user_id", user_id.toUrl());
    if (! type.isEmpty()) query.bindValue(":type", type);

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        return false;
    }
    return true;
}

bool CollectionSql::un_valid(model::ItemId user_id, QString type) {
    auto query = m_con->query();

    QStringList cond;
    cond << "user_id = :user_id";
    if (! type.isEmpty()) cond << "type = :type";

    query.prepare(QStringLiteral(R"(
UPDATE %1
SET type = 'invalid'
WHERE %2
)")
                      .arg(m_table)
                      .arg(cond.join(" AND ")));

    query.bindValue(":user_id", user_id.toUrl());
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