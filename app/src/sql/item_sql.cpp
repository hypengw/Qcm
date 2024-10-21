#include "Qcm/sql/item_sql.h"

#include <asio/bind_executor.hpp>
#include <asio/use_awaitable.hpp>
#include "asio_qt/qt_sql.h"
#include "core/str_helper.h"
#include "core/strv_helper.h"
#include "core/qstr_helper.h"

namespace qcm
{
ItemSql::ItemSql(rc<helper::SqlConnect> con)
    : m_album_table("album"),
      m_artist_table("artist"),
      m_album_artist_table("album_artist"),
      m_con(con) {
    asio::dispatch(m_con->get_executor(), [this] {
        create_album_table();
        create_artist_table();
        create_album_artist_table();
    });
}
ItemSql::~ItemSql() {}

void ItemSql::create_album_table() {
    m_con->db().transaction();

    auto migs = m_con->generate_column_migration(m_album_table,
                                                 u"itemId",
                                                 model::Album::staticMetaObject,
                                                 std::array { "full INTEGER DEFAULT 0"sv });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->db().rollback();
            return;
        }
    }
    if (! m_con->db().commit()) {
        ERROR_LOG("{}", m_con->error_str());
    }
}

void ItemSql::create_artist_table() {
    m_con->db().transaction();

    auto migs = m_con->generate_column_migration(m_artist_table,
                                                 u"itemId",
                                                 model::Artist::staticMetaObject,
                                                 std::array { "full INTEGER DEFAULT 0"sv });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->db().rollback();
            return;
        }
    }
    if (! m_con->db().commit()) {
        ERROR_LOG("{}", m_con->error_str());
    }
}

void ItemSql::create_album_artist_table() {
    m_con->db().transaction();

    auto migs = m_con->generate_column_migration(m_album_artist_table,
                                                 uR"(
CREATE TABLE IF NOT EXISTS %1 (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    albumId TEXT NOT NULL,
    artistId TEXT NOT NULL,
    UNIQUE(albumId, artistId)
);
)"_s.arg(m_album_artist_table),
                                                 std::array { "id"s, "albumId"s, "artistId"s });

    QSqlQuery q = m_con->query();

    for (auto el : migs) {
        if (! q.exec(el)) {
            ERROR_LOG("{}", q.lastError().text());
            m_con->db().rollback();
            return;
        }
    }
    if (! m_con->db().commit()) {
        ERROR_LOG("{}", m_con->error_str());
    }
}

auto ItemSql::get_executor() -> QtExecutor& { return m_con->get_executor(); }
auto ItemSql::con() const -> rc<helper::SqlConnect> { return m_con; }

auto ItemSql::insert(std::span<const model::Album> items,
                     const std::set<std::string>&  on_update) -> asio::awaitable<bool> {
    auto insert_helper =
        m_con->generate_insert_helper(m_album_table,
                                      u"itemId"_s,
                                      model::Album::staticMetaObject,
                                      items,
                                      on_update,
                                      { { u"itemId"_s, [](const QVariant& v) -> QVariant {
                                             auto id = v.value<model::ItemId>();
                                             return id.toUrl();
                                         } } });

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        co_return false;
    }

    co_return true;
}

auto ItemSql::insert(std::span<const model::Artist> items,
                     const std::set<std::string>&   on_update) -> asio::awaitable<bool> {
    auto insert_helper =
        m_con->generate_insert_helper(m_artist_table,
                                      u"itemId"_s,
                                      model::Artist::staticMetaObject,
                                      items,
                                      on_update,
                                      { { u"itemId"_s, [](const QVariant& v) -> QVariant {
                                             auto id = v.value<model::ItemId>();
                                             return id.toUrl();
                                         } } });

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        co_return false;
    }

    co_return true;
}

} // namespace qcm
