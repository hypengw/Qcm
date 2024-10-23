#include "Qcm/sql/item_sql.h"

#include <asio/bind_executor.hpp>
#include <asio/use_awaitable.hpp>
#include "asio_qt/qt_sql.h"
#include "core/str_helper.h"
#include "core/strv_helper.h"
#include "core/qstr_helper.h"

namespace qcm
{
namespace
{
auto get_song_ignore() -> std::set<std::string_view> { return { "source"sv, "sourceId"sv }; }
auto item_id_converter(const QVariant& v) -> QVariant {
    auto id = v.value<model::ItemId>();
    return id.toUrl();
}
} // namespace

ItemSql::ItemSql(rc<helper::SqlConnect> con)
    : m_album_table("album"),
      m_artist_table("artist"),
      m_song_table("song"),
      m_album_artist_table("album_artist"),
      m_song_artist_table("song_artist"),
      m_con(con) {
    asio::dispatch(m_con->get_executor(), [this] {
        create_album_table();
        create_artist_table();
        create_song_table();
        create_album_artist_table();
        create_song_artist_table();
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

void ItemSql::create_song_table() {
    m_con->db().transaction();

    auto migs = m_con->generate_column_migration(m_song_table,
                                                 u"itemId",
                                                 model::Song::staticMetaObject,
                                                 std::array { "full INTEGER DEFAULT 0"sv },
                                                 get_song_ignore());

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

void ItemSql::create_song_artist_table() {
    m_con->db().transaction();

    auto migs = m_con->generate_column_migration(m_song_artist_table,
                                                 uR"(
CREATE TABLE IF NOT EXISTS %1 (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    songId TEXT NOT NULL,
    artistId TEXT NOT NULL,
    UNIQUE(songId, artistId)
);
)"_s.arg(m_song_artist_table),
                                                 std::array { "id"s, "songId"s, "artistId"s });

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
    DEBUG_LOG("start insert album, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(m_album_table,
                                                       u"itemId"_s,
                                                       model::Album::staticMetaObject,
                                                       items,
                                                       on_update,
                                                       { { u"itemId"_s, item_id_converter } });

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    m_con->db().transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->db().rollback();
        co_return false;
    }
    m_con->db().commit();
    DEBUG_LOG("end insert");
    co_return true;
}

auto ItemSql::insert(std::span<const model::Artist> items,
                     const std::set<std::string>&   on_update) -> asio::awaitable<bool> {
    DEBUG_LOG("start insert artist, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(m_artist_table,
                                                       u"itemId"_s,
                                                       model::Artist::staticMetaObject,
                                                       items,
                                                       on_update,
                                                       { { u"itemId"_s, item_id_converter } });

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    m_con->db().transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->db().rollback();
        co_return false;
    }
    m_con->db().commit();
    DEBUG_LOG("end insert");
    co_return true;
}
auto ItemSql::insert(std::span<const model::Song> items,
                     const std::set<std::string>& on_update) -> asio::awaitable<bool> {
    DEBUG_LOG("start insert song, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(
        m_song_table,
        u"itemId"_s,
        model::Song::staticMetaObject,
        items,
        on_update,
        { { u"itemId"_s, item_id_converter }, { u"albumId"_s, item_id_converter } },
        get_song_ignore());

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    insert_helper.bind(query);

    m_con->db().transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->db().rollback();
        co_return false;
    }
    m_con->db().commit();
    DEBUG_LOG("end insert");
    co_return true;
}

auto ItemSql::insert_album_artist(std::span<const IdPair> ids) -> asio::awaitable<bool> {
    QVariantList albumIds, artistIds;
    for (auto& el : ids) {
        albumIds << std::get<0>(el).toUrl();
        artistIds << std::get<1>(el).toUrl();
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();

    query.prepare(uR"(
INSERT OR IGNORE INTO %1 (albumId, artistId) VALUES (:albumId, :artistId);
)"_s.arg(m_album_artist_table));
    query.bindValue(":albumId", albumIds);
    query.bindValue(":artistId", artistIds);
    m_con->db().transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->db().rollback();
        co_return false;
    }
    m_con->db().commit();
    co_return true;
}

auto ItemSql::insert_song_artist(std::span<const IdPair> ids) -> asio::awaitable<bool> {
    QVariantList songIds, artistIds;
    for (auto& el : ids) {
        songIds << std::get<0>(el).toUrl();
        artistIds << std::get<1>(el).toUrl();
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();

    query.prepare(uR"(
INSERT OR IGNORE INTO %1 (songId, artistId) VALUES (:songId, :artistId);
)"_s.arg(m_song_artist_table));
    query.bindValue(":songId", songIds);
    query.bindValue(":artistId", artistIds);
    m_con->db().transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->db().rollback();
        co_return false;
    }
    m_con->db().commit();
    co_return true;
}

} // namespace qcm