#include "Qcm/sql/cache_sql.h"

#include <asio/use_awaitable.hpp>
#include <asio/bind_executor.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>

#include "qcm_interface/path.h"
#include "qcm_interface/type.h"
#include "asio_qt/qt_sql.h"
#include "core/log.h"

using namespace qcm;

namespace
{
CacheSql::Item query_to_item(const QSqlQuery& q) {
    auto i_key            = q.record().indexOf("key");
    auto i_content_type   = q.record().indexOf("content_type");
    auto i_content_length = q.record().indexOf("content_length");
    return CacheSql::Item {
        .key            = convert_from<std::string>(q.value(i_key).toString()),
        .content_type   = convert_from<std::string>(q.value(i_content_type).toString()),
        .content_length = q.value(i_content_length).toULongLong(),
    };
}
} // namespace

CacheSql::CacheSql(std::string_view table, i64 limit, rc<helper::SqlConnect> con)
    : m_con(con), m_table(convert_from<QString>(table)), m_limit(limit) {
    asio::dispatch(m_con->get_executor(), [this]() {
        try_connect();
    });
}

CacheSql::~CacheSql() {}

auto CacheSql::get_executor() -> asio::any_io_executor { return get_qexecutor(); }
auto CacheSql::get_qexecutor() -> QtExecutor& { return m_con->get_executor(); }

bool CacheSql::is_reached_limit() { return m_limit > 0 && m_total > m_limit; }

void CacheSql::try_connect() {
    if (m_con->is_open()) {
        create_table();
        m_total = total_size_sync();
    }
}

bool CacheSql::create_table() {
    QSqlQuery q = m_con->query();
    q.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS %1 (key text not null primary key, "
                             "content_type text, content_length integer, timestamp integer);")
                  .arg(m_table));
    return q.exec();
}

void CacheSql::set_limit(i64 limit) {
    asio::dispatch(get_qexecutor(), [this, limit]() {
        if (std::exchange(m_limit, limit) != limit) {
            if (is_reached_limit()) trigger_try_clean();
        }
    });
}

void CacheSql::set_clean_cb(clean_cb_t f) { m_clean_cb = std::move(f); }

task<std::optional<CacheSql::Item>> CacheSql::get(std::string key) {
    co_await asio::post(asio::bind_executor(get_qexecutor(), asio::use_awaitable));
    {
        QSqlQuery q = m_con->query();
        q.prepare(QStringLiteral("SELECT * FROM %1 WHERE key = :key").arg(m_table));
        q.bindValue(":key", convert_from<QString>(key));
        if (q.exec()) {
            while (q.next()) {
                auto item = query_to_item(q);

                // update timestamp
                {
                    QSqlQuery q = m_con->query();
                    q.prepare(
                        QStringLiteral("UPDATE %1 SET timestamp = :timestamp WHERE key = :key;")
                            .arg(m_table));
                    q.bindValue(":key", convert_from<QString>(key));
                    q.bindValue(":timestamp", QDateTime::currentSecsSinceEpoch());
                    q.exec();
                }
                co_return item;
            }
        }
    }

    co_return std::nullopt;
}

task<void> CacheSql::insert(Item item) {
    co_await asio::post(asio::bind_executor(get_qexecutor(), asio::use_awaitable));
    {
        QSqlQuery q = m_con->query();
        q.prepare(
            QStringLiteral("INSERT OR ABORT INTO %1 (key, content_type, content_length, timestamp) "
                           "VALUES (:key, "
                           ":content_type, :content_length, :timestamp);")
                .arg(m_table));
        q.bindValue(":key", convert_from<QString>(item.key));
        q.bindValue(":content_type", convert_from<QString>(item.content_type));
        q.bindValue(":content_length", QVariant::fromValue(item.content_length));
        q.bindValue(":timestamp", QDateTime::currentSecsSinceEpoch());
        if (q.exec()) {
            m_total += item.content_length / 1024.0;
            if (is_reached_limit()) trigger_try_clean();
        }
    }
    co_return;
}

task<void> CacheSql::remove(std::string key) {
    co_await asio::post(asio::bind_executor(get_qexecutor(), asio::use_awaitable));
    {
        QSqlQuery q = m_con->query();
        q.prepare(QStringLiteral("DELETE FROM %1 WHERE key = :key").arg(m_table));
        q.bindValue(":key", convert_from<QString>(key));
        q.exec();
    }
    co_return;
}

task<void> CacheSql::remove(std::span<std::string> keys) {
    co_await asio::post(asio::bind_executor(get_qexecutor(), asio::use_awaitable));
    {
        QSqlQuery   q = m_con->query();
        QStringList placeholders;
        for (usize i = 0; i < keys.size(); ++i) {
            placeholders << QStringLiteral(":key%1").arg(i); // :key0, :key1, :key2
        }

        q.prepare(QStringLiteral("DELETE FROM %1 WHERE key IN (%2)")
                      .arg(m_table)
                      .arg(placeholders.join(", ")));
        for (usize i = 0; i < keys.size(); ++i) {
            q.bindValue(QStringLiteral(":key%1").arg(i), convert_from<QString>(keys[i]));
        }
        q.exec();
    }
    co_return;
}

task<std::optional<CacheSql::Item>> CacheSql::lru() {
    co_await asio::post(asio::bind_executor(get_qexecutor(), asio::use_awaitable));
    {
        QSqlQuery q = m_con->query();
        q.prepare(QStringLiteral("SELECT * FROM %1 WHERE timestamp = (SELECT "
                                 "MIN(timestamp) FROM %1)")
                      .arg(m_table));
        if (q.exec()) {
            while (q.next()) {
                co_return query_to_item(q);
            }
        }
    }
    co_return std::nullopt;
}

usize CacheSql::total_size_sync() {
    QSqlQuery q = m_con->query();
    q.prepare(QStringLiteral("SELECT SUM(content_length)/1024 FROM %1").arg(m_table));
    if (q.exec()) {
        while (q.next()) {
            auto total = q.value(0).toDouble();
            return total > 0 ? (usize)total : 0;
        }
    }
    return 0;
}

task<usize> CacheSql::total_size() {
    co_await asio::post(asio::bind_executor(get_qexecutor(), asio::use_awaitable));
    co_return total_size_sync();
}

task<std::vector<CacheSql::Item>> CacheSql::get_all() {
    co_await asio::post(asio::bind_executor(get_qexecutor(), asio::use_awaitable));
    std::vector<CacheSql::Item> out;
    {
        QSqlQuery q = m_con->query();
        q.prepare(QStringLiteral("SELECT * FROM %1").arg(m_table));
        if (q.exec()) {
            while (q.next()) {
                out.emplace_back(query_to_item(q));
            }
        }
    }
    co_return out;
}

task<void> CacheSql::try_clean() {
    co_await asio::post(asio::bind_executor(get_qexecutor(), asio::use_awaitable));
    m_total    = co_await total_size();
    auto limit = m_limit * 0.9;
    while (limit > 0 && m_total > limit) {
        if (auto item = co_await lru(); item) {
            co_await remove(item->key);
            m_total -= item->content_length / 1024.0;
            if (m_clean_cb) m_clean_cb(item->key);
        } else {
            break;
        }
    }
}

void CacheSql::trigger_try_clean() {
    auto self = shared_from_this();
    asio::co_spawn(
        get_qexecutor(),
        [self]() -> task<void> {
            co_await self->try_clean();
            co_return;
        },
        asio::detached);
}
