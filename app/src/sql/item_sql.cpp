#include "Qcm/sql/item_sql.h"

#include <unordered_set>
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
      m_playlist_table("playlist"),
      m_playlist_song_table("playlist_song"),
      m_con(con) {
    asio::dispatch(m_con->get_executor(), [this] {
        create_album_table();
        create_artist_table();
        create_song_table();
        create_album_artist_table();
        create_song_artist_table();
        create_playlist_table();
        create_playlist_song_table();
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

void ItemSql::create_playlist_table() {
    m_con->db().transaction();

    auto migs = m_con->generate_column_migration(m_playlist_table,
                                                 u"itemId",
                                                 model::Playlist::staticMetaObject,
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
    %2,
    UNIQUE(albumId, artistId)
);
)"_s.arg(m_album_artist_table)
                                                     .arg(m_con->EditTimeColumnQSV),
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
    %2,
    UNIQUE(songId, artistId)
);
)"_s.arg(m_song_artist_table)
                                                     .arg(m_con->EditTimeColumnQSV),
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

void ItemSql::create_playlist_song_table() {
    m_con->db().transaction();

    auto migs = m_con->generate_column_migration(m_playlist_song_table,
                                                 uR"(
CREATE TABLE IF NOT EXISTS %1 (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    songId TEXT NOT NULL,
    playlistId TEXT NOT NULL,
    orderIdx INTEGER,
    %2,
    UNIQUE(songId, playlistId)
);
)"_s.arg(m_playlist_song_table)
                                                     .arg(m_con->EditTimeColumnQSV),
                                                 std::array { "id"s, "songId"s, "playlistId"s });

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
                     const std::set<std::string>&  on_update) -> task<bool> {
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
                     const std::set<std::string>&   on_update) -> task<bool> {
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
                     const std::set<std::string>& on_update) -> task<bool> {
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
auto ItemSql::insert(std::span<const model::Playlist> items,
                     const std::set<std::string>&     on_update) -> task<bool> {
    DEBUG_LOG("start insert playlist, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(
        m_playlist_table,
        u"itemId"_s,
        model::Playlist::staticMetaObject,
        items,
        on_update,
        { { u"itemId"_s, item_id_converter }, { u"userId"_s, item_id_converter } });

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

auto ItemSql::insert_album_artist(std::span<const IdPair> ids) -> task<bool> {
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

auto ItemSql::insert_song_artist(std::span<const IdPair> ids) -> task<bool> {
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

auto ItemSql::insert_playlist_song(i32 pos, model::ItemId playlist_id,
                                   std::span<const model::ItemId> song_ids) -> task<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();
    query.prepare(uR"(
SELECT 
    COALESCE(
        (SELECT orderIdx
         FROM playlist_song
         WHERE playlistId = :playlistId
         ORDER BY orderIdx
         LIMIT 1 OFFSET :pos - 1),
        (SELECT orderIdx
         FROM playlist_song
         WHERE playlistId = :playlistId
         ORDER BY orderIdx DESC
         LIMIT 1)
    ) AS orderIdx,
    COUNT(*) AS totalCount
FROM playlist_song
WHERE playlistId = :playlistId;
)"_s);

    query.bindValue(":pos", pos);
    query.bindValue(":playlistId", playlist_id.toUrl());

    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
        co_return false;
    } else if (query.next()) {
        u32 last  = query.value(0).toUInt();
        u32 count = query.value(1).toUInt();
        u32 limit = 100;

        m_con->db().transaction();

        query              = m_con->query();
        auto rerange_query = m_con->query();
        query.prepare(uR"(
INSERT OR REPLACE INTO %1 (playlistId, songId, orderIdx)
VALUES (:playlistId, :songId, :orderIdx);
)"_s.arg(m_playlist_song_table));
        rerange_query.prepare(uR"(
WITH OrderedSongs AS (
    SELECT songId,
           ROW_NUMBER() OVER (ORDER BY orderIdx) * :limit AS newOrderIdx
    FROM playlist_song
    WHERE playlistId = :playlistId
)
UPDATE playlist_song
SET orderIdx = (SELECT newOrderIdx FROM OrderedSongs WHERE playlist_song.songId = OrderedSongs.songId)
WHERE playlistId = :playlistId;
)"_s);
        u32 offset { last };
        for (u32 i = 0; i < song_ids.size(); i += limit - 1, offset += (limit - 1) * limit) {
            QVariantList playlist_id_list;
            QVariantList song_id_list;
            QVariantList order_idx;
            auto         n = std::min<usize>(i + limit, song_ids.size());
            for (u32 j = i, k = 1; j < n; j++, k++) {
                playlist_id_list << playlist_id.toUrl();
                song_id_list << song_ids[j].toUrl();
                order_idx << offset + k;
            }

            query.bindValue(":playlistId", playlist_id_list);
            query.bindValue(":songId", song_id_list);
            query.bindValue(":orderIdx", order_idx);
            if (! query.execBatch()) {
                ERROR_LOG("{}", query.lastError().text());
                m_con->db().rollback();
                co_return false;
            }

            rerange_query.bindValue(":limit", limit);
            rerange_query.bindValue(":playlistId", playlist_id.toUrl());
            if (! rerange_query.exec()) {
                ERROR_LOG("{}", rerange_query.lastError().text());
                m_con->db().rollback();
                co_return false;
            }
        }

        m_con->db().commit();
    } else {
    }

    co_return true;
}

auto ItemSql::table_name(Table t) const -> QStringView {
    switch (t) {
    case Table::ALBUM: return m_album_table;
    case Table::SONG: return m_song_table;
    case Table::ALBUM_ARTIST: return m_album_artist_table;
    case Table::PLAYLIST_SONG: return m_playlist_song_table;
    case Table::PLAYLIST: return m_playlist_table;
    case Table::ARTIST: return m_artist_table;
    case Table::SONG_ARTIST: return m_song_artist_table;
    }
    _assert_rel_(false);
    return {};
}
auto ItemSql::clean(const QDateTime& before, Table table) -> task<void> {
    auto cur = QDateTime::currentDateTime();

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();

    query.prepare(uR"(
DELETE FROM %1
WHERE _editTime < :before OR _editTime > :after;
)"_s);
    query.bindValue(":before", before);
    query.bindValue(":after", cur);
    if (! query.exec()) {
        ERROR_LOG("{}", query.lastError().text());
    }
}

auto ItemSql::missing(Table                          table,
                      std::span<const model::ItemId> ids) -> task<std::vector<model::ItemId>> {
    std::unordered_set<model::ItemId> include;
    std::vector<model::ItemId>        out;
    QStringList                       placeholders;
    for (usize i = 0; i < ids.size(); ++i) {
        placeholders << u":id%1"_s.arg(i);
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    query.prepare(uR"(
SELECT 
    itemId
FROM %1 
WHERE itemId IN (%2);
)"_s.arg(table_name(table))
                      .arg(placeholders.join(", ")));
    for (usize i = 0; i < ids.size(); ++i) {
        query.bindValue(placeholders[i], ids[i].toUrl());
    }
    if (query.exec()) {
        while (query.next()) {
            include.insert(model::ItemId { query.value(0).toUrl() });
        }
    } else {
        ERROR_LOG("{}", query.lastError().text());
    }
    for (auto& el : ids) {
        if (! include.contains(el)) {
            out.emplace_back(el);
        }
    }
    co_return out;
}

} // namespace qcm