#include "Qcm/collection_sql.h"

#include <QSqlError>
#include <QSqlQuery>
#include <asio/dispatch.hpp>
#include <asio/bind_executor.hpp>
#include <asio/use_awaitable.hpp>

#include "qcm_interface/path.h"
#include "core/qstr_helper.h"
#include "core/log.h"

namespace qcm
{

CollectionSql::CollectionSql(std::string_view table)
    : m_thread(),
      m_ctx(make_rc<QtExecutionContext>(&m_thread, (QEvent::Type)QEvent::registerEventType())),
      m_ex(m_ctx),
      m_table(convert_from<QString>(table)) {
    asio::dispatch(m_ex, [this]() {
        connect_db();
    });
}
CollectionSql::~CollectionSql() {}

auto CollectionSql::get_executor() -> QtExecutor& { return m_ex; }

void CollectionSql::connect_db() {
    m_db   = QSqlDatabase::addDatabase("QSQLITE", m_table);
    auto p = (data_path() / "data.db");
    m_db.setDatabaseName(p.native().c_str());

    if (m_db.open()) {
        QSqlQuery q(m_db);
        QString   createTable = QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id TEXT NOT NULL,
        item_type TEXT NOT NULL,
        item_id TEXT NOT NULL,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    )
)")
                                  .arg(m_table);
        if (! q.exec(createTable)) {
            ERROR_LOG("creating table '{}' failed: {}", m_table, q.lastError().text());
        }
    } else {
        ERROR_LOG("{}", m_db.lastError().text());
    }
}

auto CollectionSql::insert(Item item) -> asio::awaitable<void> {
    co_await asio::post(asio::bind_executor(m_ex, asio::use_awaitable));

    QSqlQuery query;
    query.prepare(QStringLiteral(R"(
    INSERT INTO collection (user_id, collection_name, item_type, item_id, created_at)
    VALUES (:user_id, :collection_name, :item_type, :item_id, :created_at)
)"));

    query.bindValue(":user_id", item.user_id.toUrl());
    query.bindValue(":item_type", item.type);
    query.bindValue(":item_id", item.item_id.toUrl());
    query.bindValue(":created_at", QDateTime::currentDateTime());

    if (query.exec()) {
    }
    co_return;
}
auto CollectionSql::select_id(model::ItemId user_id,
                              QString) -> asio::awaitable<std::vector<model::ItemId>> {
    co_await asio::post(asio::bind_executor(m_ex, asio::use_awaitable));

    QSqlQuery query;
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

} // namespace qcm