#include "Qcm/sql/item_sql.h"

#include <unordered_set>
#include <asio/bind_executor.hpp>
#include <asio/use_awaitable.hpp>
#include "asio_qt/qt_sql.h"
#include "core/str_helper.h"
#include "core/strv_helper.h"
#include "core/qstr_helper.h"
#include "qcm_interface/sql/meta_sql.h"
#include "Qcm/query/query_load.h"
#include "platform/platform.h"

namespace qcm
{
namespace
{

constexpr helper::SqlColumn LibraryIdColumn {
    .name    = "libraryId",
    .type    = "INTEGER",
    .notnull = 1,
    .foreign = helper::SqlForeignKey { "libraryId", "library", "libraryId" },
};

template<typename T, typename F>
    requires(! std::is_trivially_copyable_v<std::remove_cvref_t<F>>)
void bindLibraryId(T& query, F&& val) {
    query.bindValue(":libraryId", std::forward<F>(val));
}

template<typename T, typename F>
    requires std::is_trivially_copyable_v<F>
void bindLibraryId(T& query, F val) {
    query.bindValue(":libraryId", val);
}

auto get_song_ignore() -> std::set<std::string_view> { return { "source"sv, "sourceId"sv }; }
auto song_ignore(const std::set<std::string>& in) {
    auto out = in;
    out.insert("^source");
    out.insert("^sourceId");
    return out;
}
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
      m_mix_table("playlist"),
      m_mix_song_table("playlist_song"),
      m_radio_table("radio"),
      m_program_table("program"),
      m_radio_program_table("radio_program"),
      m_library_table("library"),
      m_con(con) {
    asio::dispatch(m_con->get_executor(), [this] {
        plt::set_thread_name("sql_item");

        create_library_table();
        create_album_table();
        create_artist_table();
        create_song_table();
        create_album_artist_table();
        create_song_artist_table();
        create_mix_table();
        create_mix_song_table();
        create_radio_table();
        create_program_table();
        create_radio_program_table();
    });
}
ItemSql::~ItemSql() {}

void ItemSql::create_album_table() {
    auto opts = helper::SqlMetaOptions {};
    opts.foreigns.insert({ LibraryIdColumn.name, *LibraryIdColumn.foreign });
    opts.fts_columns = { "name"s };
    auto migs        = m_con->generate_meta_migration(m_album_table,
                                               u"",
                                               model::Album::staticMetaObject,
                                                      { { "itemId"sv, "libraryId"sv } },
                                               opts);

    m_con->exec_with_transaction(migs);
}

void ItemSql::create_artist_table() {
    auto opts = helper::SqlMetaOptions {};
    opts.foreigns.insert({ LibraryIdColumn.name, *LibraryIdColumn.foreign });
    auto migs = m_con->generate_meta_migration(m_artist_table,
                                               u"",
                                               model::Artist::staticMetaObject,
                                               { { "itemId"sv, "libraryId"sv } },
                                               opts);
    m_con->exec_with_transaction(migs);
}

void ItemSql::create_song_table() {
    auto opts = helper::SqlMetaOptions {};
    opts.foreigns.insert({ LibraryIdColumn.name, *LibraryIdColumn.foreign });
    opts.fts_columns = { "name"s };
    opts.exclude     = get_song_ignore();
    auto migs        = m_con->generate_meta_migration(
        m_song_table, u"", model::Song::staticMetaObject, { { "itemId"sv, "libraryId"sv } }, opts);
    m_con->exec_with_transaction(migs);
}

void ItemSql::create_mix_table() {
    auto opts = helper::SqlMetaOptions {};
    opts.foreigns.insert({ LibraryIdColumn.name, *LibraryIdColumn.foreign });
    auto migs = m_con->generate_meta_migration(
        m_mix_table, u"", model::Mix::staticMetaObject, { { "itemId"sv, "libraryId"sv } }, opts);

    m_con->exec_with_transaction(migs);
}

void ItemSql::create_radio_table() {
    auto opts = helper::SqlMetaOptions {};
    opts.foreigns.insert({ LibraryIdColumn.name, *LibraryIdColumn.foreign });
    auto migs = m_con->generate_meta_migration(m_radio_table,
                                               u"",
                                               model::Radio::staticMetaObject,
                                               { { "itemId"sv, "libraryId"sv } },
                                               opts);
    m_con->exec_with_transaction(migs);
}

void ItemSql::create_program_table() {
    auto opts = helper::SqlMetaOptions {};
    opts.foreigns.insert({ LibraryIdColumn.name, *LibraryIdColumn.foreign });
    auto migs = m_con->generate_meta_migration(m_program_table,
                                               u"",
                                               model::Program::staticMetaObject,
                                               { { "itemId"sv, "libraryId"sv } },
                                               opts);
    m_con->exec_with_transaction(migs);
}

void ItemSql::create_album_artist_table() {
    auto migs = m_con->generate_column_migration(
        m_album_artist_table,
        std::array {
            LibraryIdColumn,
            helper::SqlColumn { .name = "albumId", .type = "TEXT", .notnull = 1 },
            helper::SqlColumn { .name = "artistId", .type = "TEXT", .notnull = 1 },
            m_con->EditTimeColumn,
        },
        { helper::SqlUnique { "albumId"sv, "artistId"sv, LibraryIdColumn.name } });

    m_con->exec_with_transaction(migs);
}

void ItemSql::create_song_artist_table() {
    auto migs = m_con->generate_column_migration(
        m_song_artist_table,
        std::array {
            LibraryIdColumn,
            helper::SqlColumn { .name = "songId", .type = "TEXT", .notnull = 1 },
            helper::SqlColumn { .name = "artistId", .type = "TEXT", .notnull = 1 },
            m_con->EditTimeColumn,
        },
        { { "songId"sv, "artistId"sv, LibraryIdColumn.name } });

    m_con->exec_with_transaction(migs);
}

void ItemSql::create_mix_song_table() {
    auto migs = m_con->generate_column_migration(
        m_mix_song_table,
        std::array {
            LibraryIdColumn,
            helper::SqlColumn { .name = "songId", .type = "TEXT", .notnull = 1 },
            helper::SqlColumn { .name = "playlistId", .type = "TEXT", .notnull = 1 },
            helper::SqlColumn { .name = "orderIdx", .type = "INTEGER" },
            helper::SqlColumn { .name = "removed", .type = "INTEGER", .dflt_value = "0"s },
            m_con->EditTimeColumn,
        },
        { { "songId"sv, "playlistId"sv, LibraryIdColumn.name } });

    m_con->exec_with_transaction(migs);
}
void ItemSql::create_radio_program_table() {
    auto migs = m_con->generate_column_migration(
        m_radio_program_table,
        std::array {
            LibraryIdColumn,
            helper::SqlColumn { .name = "radioId", .type = "TEXT", .notnull = 1 },
            helper::SqlColumn { .name = "programId", .type = "TEXT", .notnull = 1 },
            m_con->EditTimeColumn,
        },
        { { "radioId"sv, "programId"sv, LibraryIdColumn.name } });

    m_con->exec_with_transaction(migs);
}

void ItemSql::create_library_table() {
    auto migs =
        m_con->generate_meta_migration(m_library_table,
                                       u"libraryId",
                                       model::Library::staticMetaObject,
                                       { helper::SqlUnique { "providerId"sv, "nativeId"sv } });

    m_con->exec_with_transaction(migs);
}

auto ItemSql::get_executor() -> QtExecutor& { return m_con->get_executor(); }
auto ItemSql::con() const -> rc<helper::SqlConnect> { return m_con; }

auto ItemSql::library_id(i64 provider_id, const QString& native_id) -> task<i64> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    query.prepare(R"(
SELECT {1} 
FROM {0}
WHERE providerId = {2} AND nativeId = :nativeId
)",
                  m_library_table,
                  LibraryIdColumn.name,
                  provider_id);
    query.bindValue(":nativeId", native_id);

    if (query.exec()) {
        if (query.next()) {
            co_return query.value(0).toLongLong();
        }
    }

    co_return -1;
}
auto ItemSql::library_id_list() -> task<std::vector<i64>> {
    std::vector<i64> out;
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    query.prepare(R"(
SELECT {1} 
FROM {0} 
ORDER BY {1};
)",
                  m_library_table,
                  LibraryIdColumn.name);

    if (query.exec()) {
        while (query.next()) {
            out.push_back(query.value(0).toLongLong());
        }
    }

    co_return out;
}

auto ItemSql::library_list() -> task<std::vector<model::Library>> {
    std::vector<model::Library> out;
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    query.prepare(R"(
SELECT {1} 
FROM {0} 
ORDER BY {1};
)",
                  m_library_table,
                  LibraryIdColumn.name);

    if (query.exec()) {
        while (query.next()) {
        }
    }
    co_return out;
}

auto ItemSql::create_library(model::Library lib) -> task<model::Library> {
    std::set<std::string> columns;
    if (lib.libraryId == -1) {
        // let sql generate
        columns.insert({ "^libraryId" });
    }
    auto insert_helper = m_con->generate_insert_helper(m_library_table,
                                                       { "providerId", "nativeId" },
                                                       model::Library::staticMetaObject,
                                                       std::span<const model::Library> { &lib, 1 },
                                                       columns,
                                                       columns);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    m_con->with_transaction([&insert_helper, &lib](auto& query) {
        insert_helper.bind(query);
        bool ok = true;
        do {
            ok = query.execBatch();
            YCORE_BREAK_ON(! ok);
            ok = query.exec("SELECT last_insert_rowid();");
            if (ok) {
                lib.libraryId = query.lastInsertId().toLongLong();
            }
        } while (0);
        return ok;
    });
    co_return lib;
}
auto ItemSql::delete_library(i64 lib_id) -> task<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    m_con->with_transaction([this, lib_id](auto& query) {
        query.prepare("DELETE FROM {0} WHERE libraryId = {1};", m_library_table, lib_id);
        return query.exec();
    });
    co_return true;
}

auto ItemSql::insert(std::span<const model::Album> items, ListParam columns, ListParam on_update)
    -> task<bool> {
    DEBUG_LOG("start insert album, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(m_album_table,
                                                       { "itemId"s, LibraryIdColumn.name },
                                                       model::Album::staticMetaObject,
                                                       items,
                                                       columns,
                                                       on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    m_con->with_transaction([&insert_helper](helper::SqlQuery& q) {
        insert_helper.bind(q);
        return q.execBatch();
    });

    DEBUG_LOG("end insert");
    co_return true;
}

auto ItemSql::insert(std::span<const model::Artist> items, ListParam columns,
                     const std::set<std::string>& on_update) -> task<bool> {
    DEBUG_LOG("start insert artist, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(m_artist_table,
                                                       { "itemId"s, LibraryIdColumn.name },
                                                       model::Artist::staticMetaObject,
                                                       items,
                                                       columns,
                                                       on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    m_con->with_transaction([&insert_helper](helper::SqlQuery& q) {
        insert_helper.bind(q);
        return q.execBatch();
    });
    DEBUG_LOG("end insert");
    co_return true;
}
auto ItemSql::insert(std::span<const model::Song> items, ListParam columns, ListParam on_update)
    -> task<bool> {
    DEBUG_LOG("start insert song, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(m_song_table,
                                                       { "itemId"s, LibraryIdColumn.name },
                                                       model::Song::staticMetaObject,
                                                       items,
                                                       song_ignore(columns),
                                                       on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    m_con->with_transaction([&insert_helper](helper::SqlQuery& q) {
        insert_helper.bind(q);
        return q.execBatch();
    });
    DEBUG_LOG("end insert");
    co_return true;
}
auto ItemSql::insert(std::span<const model::Mix> items, ListParam columns,
                     const std::set<std::string>& on_update) -> task<bool> {
    DEBUG_LOG("start insert playlist, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(m_mix_table,
                                                       { "itemId"s, LibraryIdColumn.name },
                                                       model::Mix::staticMetaObject,
                                                       items,
                                                       columns,
                                                       on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    m_con->with_transaction([&insert_helper](helper::SqlQuery& q) {
        insert_helper.bind(q);
        return q.execBatch();
    });
    DEBUG_LOG("end insert");
    co_return true;
}
auto ItemSql::insert(std::span<const model::Radio> items, ListParam columns,
                     const std::set<std::string>& on_update) -> task<bool> {
    DEBUG_LOG("start insert radio, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(m_radio_table,
                                                       { "itemId"s, LibraryIdColumn.name },
                                                       model::Radio::staticMetaObject,
                                                       items,
                                                       columns,
                                                       on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    m_con->with_transaction([&insert_helper](helper::SqlQuery& q) {
        insert_helper.bind(q);
        return q.execBatch();
    });
    DEBUG_LOG("end insert");
    co_return true;
}
auto ItemSql::insert(std::span<const model::Program> items, ListParam columns,
                     const std::set<std::string>& on_update) -> task<bool> {
    DEBUG_LOG("start insert program, {}", items.size());
    auto insert_helper = m_con->generate_insert_helper(m_program_table,
                                                       { "itemId"s, LibraryIdColumn.name },
                                                       model::Program::staticMetaObject,
                                                       items,
                                                       columns,
                                                       on_update);

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    m_con->with_transaction([&insert_helper](helper::SqlQuery& q) {
        insert_helper.bind(q);
        return q.execBatch();
    });
    DEBUG_LOG("end insert");
    co_return true;
}

auto ItemSql::insert(std::span<const model::Library> items, ListParam columns,
                     const std::set<std::string>& on_update) -> task<bool> {
    Q_UNUSED(columns);
    Q_UNUSED(on_update);
    DEBUG_LOG("start insert program, {}", items.size());
    DEBUG_LOG("end insert");
    co_return true;
}

auto ItemSql::insert_album_artist(std::span<const RelationInsertId> ids) -> task<bool> {
    QVariantList library_ids, album_ids, artist_ids;
    for (auto& el : ids) {
        library_ids << el.libraryid;
        album_ids << el.first.toUrl();
        artist_ids << el.second.toUrl();
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();

    query.prepare_sv(fmt::format(R"(
INSERT OR IGNORE INTO {0} ({1}, albumId, artistId) VALUES (:{1}, :albumId, :artistId);
)",
                                 m_album_artist_table,
                                 LibraryIdColumn.name));
    bindLibraryId(query, library_ids);
    query.bindValue(":albumId", album_ids);
    query.bindValue(":artistId", artist_ids);
    m_con->transaction();
    if (! query.execBatch()) {
        ERROR_LOG("{}", query.lastError().text());
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    co_return true;
}

auto ItemSql::insert_song_artist(std::span<const RelationInsertId> ids) -> task<bool> {
    QVariantList library_ids, song_ids, artist_ids;
    for (auto& el : ids) {
        library_ids << el.libraryid;
        song_ids << el.first.toUrl();
        artist_ids << el.second.toUrl();
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    co_return m_con->with_transaction(
        [this, &library_ids, &song_ids, &artist_ids](helper::SqlQuery& query) {
            query.prepare(uR"(
INSERT OR IGNORE INTO %1 (songId, artistId) VALUES (:songId, :artistId);
)"_s.arg(m_song_artist_table));
            bindLibraryId(query, library_ids);
            query.bindValue(":songId", song_ids);
            query.bindValue(":artistId", artist_ids);

            return query.execBatch();
        });
}

auto ItemSql::insert_radio_program(std::span<const RelationInsertId> ids) -> task<bool> {
    QVariantList library_ids, radioIds, programIds;
    for (auto& el : ids) {
        library_ids << el.libraryid;
        radioIds << el.first.toUrl();
        programIds << el.second.toUrl();
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();

    query.prepare(uR"(
INSERT OR IGNORE INTO %1 (radioId, programId) VALUES (:radioId, :programId);
)"_s.arg(m_song_artist_table));
    bindLibraryId(query, library_ids);
    query.bindValue(":radioId", radioIds);
    query.bindValue(":programId", programIds);
    m_con->transaction();
    if (! query.execBatch()) {
        m_con->rollback();
        co_return false;
    }
    m_con->commit();
    co_return true;
}

auto ItemSql::insert_mix_song(i32 lib_id, u32 last, u32 count, const model::ItemId& playlist_id,
                              std::span<const model::ItemId> song_ids) -> bool {
    Q_UNUSED(count);
    u32 limit = 100;

    auto query         = m_con->query();
    auto rerange_query = m_con->query();
    query.prepare(R"(
INSERT OR REPLACE INTO {0} ({1}, playlistId, songId, orderIdx)
VALUES (:{1}, :playlistId, :songId, :orderIdx);
)",
                  m_mix_song_table,
                  LibraryIdColumn.name);
    rerange_query.prepare(R"(
WITH OrderedSongs AS (
    SELECT songId,
           ROW_NUMBER() OVER (ORDER BY orderIdx) * :limit AS newOrderIdx
    FROM playlist_song
    WHERE playlistId = :playlistId
)
UPDATE playlist_song
SET orderIdx = (SELECT newOrderIdx FROM OrderedSongs WHERE playlist_song.songId = OrderedSongs.songId)
WHERE playlistId = :playlistId AND {0} = :{0};
)",
                          LibraryIdColumn.name);
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

        bindLibraryId(query, lib_id);
        query.bindValue(":playlistId", playlist_id_list);
        query.bindValue(":songId", song_id_list);
        query.bindValue(":orderIdx", order_idx);
        if (! query.execBatch()) {
            return false;
        }

        bindLibraryId(rerange_query, lib_id);
        rerange_query.bindValue(":limit", limit);
        rerange_query.bindValue(":playlistId", playlist_id.toUrl());
        if (! rerange_query.exec()) {
            return false;
        }
    }
    return true;
}

auto ItemSql::insert_mix_song(i32 lib_id, i32 pos, model::ItemId playlist_id,
                              std::span<const model::ItemId> song_ids) -> task<bool> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();
    query.prepare(R"(
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
WHERE playlistId = :playlistId AND {0} = :{0};
)",
                  LibraryIdColumn.name);
    bindLibraryId(query, lib_id);
    query.bindValue(":pos", pos);
    query.bindValue(":playlistId", playlist_id.toUrl());

    if (! query.exec()) {
        co_return false;
    } else if (query.next()) {
        u32 last  = query.value(0).toUInt();
        u32 count = query.value(1).toUInt();
        if (pos == 0) last = 0;
        m_con->transaction();
        if (insert_mix_song(lib_id, last, count, playlist_id, song_ids)) {
            m_con->commit();
        } else {
            m_con->rollback();
            co_return false;
        }
    } else {
    }

    co_return true;
}
auto ItemSql::remove_mix_song(i32 lib_id, model::ItemId mix_id,
                              std::span<const model::ItemId> song_ids) -> task<bool> {
    QStringList placeholders;
    for (usize i = 0; i < song_ids.size(); ++i) {
        placeholders << u":id%1"_s.arg(i);
    }
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = con()->query();
    query.prepare_sv(fmt::format(R"(
DELETE FROM {0}
WHERE playlistId = :playlistId AND songId IN ({1});
)",
                                 m_mix_song_table,
                                 placeholders.join(",")));

    query.bindValue(":playlistId", mix_id.toUrl());
    for (usize i = 0; i < song_ids.size(); ++i) {
        query.bindValue(placeholders[i], song_ids[i].toUrl());
    }

    if (! query.exec()) {
        co_return false;
    }
    co_return true;
}

auto ItemSql::refresh_mix_song(i32 lib_id, i32 pos, model::ItemId mix_id,
                               std::span<const model::ItemId> song_ids) -> task<bool> {
    Q_UNUSED(pos);
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    m_con->transaction();

    auto query = con()->query();

    query.prepare(R"(
DELETE FROM {0}
WHERE playlistId = :playlistId AND {1} = :{1};
)",
                  m_mix_song_table,
                  LibraryIdColumn.name);
    query.bindValue(":playlistId", mix_id.toUrl());

    if (! query.exec()) {
        m_con->rollback();
        co_return false;
    }

    if (! insert_mix_song(lib_id, 0, 0, mix_id, song_ids)) {
        m_con->rollback();
        co_return false;
    }

    if (! m_con->commit()) {
        co_return false;
    }

    co_return true;
}
auto ItemSql::select_mix(const model::ItemId& user_id, qint32 special_type)
    -> task<std::optional<model::Mix>> {
    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = con()->query();

    query.prepare_sv(fmt::format(R"(
SELECT 
    {1}
FROM {0}
WHERE userId = :userId AND specialType = :specialType
LIMIT 1;
)",
                                 m_mix_table,
                                 model::Mix::sql().select));
    query.bindValue(":userId", user_id.toUrl());
    query.bindValue(":specialType", special_type);
    query.exec();

    if (query.next()) {
        model::Mix m;
        int        i = 0;
        query::load_query(query, m, i);
        co_return m;
    }
    co_return std::nullopt;
}

auto ItemSql::table_name(Table t) const -> QStringView {
    switch (t) {
    case Table::ALBUM: return m_album_table;
    case Table::SONG: return m_song_table;
    case Table::ALBUM_ARTIST: return m_album_artist_table;
    case Table::PLAYLIST_SONG: return m_mix_song_table;
    case Table::PLAYLIST: return m_mix_table;
    case Table::ARTIST: return m_artist_table;
    case Table::SONG_ARTIST: return m_song_artist_table;
    case Table::RADIO: return m_radio_table;
    case Table::LIBRARY: return m_library_table;
    }
    _assert_rel_(false);
    return {};
}
auto ItemSql::clean(const QDateTime& before, Table table) -> task<void> {
    auto cur = QDateTime::currentDateTimeUtc();

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));
    auto query = m_con->query();

    query.prepare(uR"(
DELETE FROM %1
WHERE _editTime < :before OR _editTime > :after;
)"_s.arg(table_name(table)));
    query.bindValue(":before", before);
    query.bindValue(":after", cur);
    query.exec();
}

auto ItemSql::missing(std::span<const model::ItemId> ids, Table table, std::optional<Table> join,
                      ListParam not_null) -> task<std::vector<model::ItemId>> {
    std::unordered_set<model::ItemId> include;
    std::vector<model::ItemId>        out;
    QStringList                       placeholders;
    for (usize i = 0; i < ids.size(); ++i) {
        placeholders << u":id%1"_s.arg(i);
    }

    co_await asio::post(asio::bind_executor(get_executor(), asio::use_awaitable));

    auto query = m_con->query();
    query.prepare_sv(fmt::format(
        R"(
SELECT 
    {0}.itemId
FROM {0}
{1}
WHERE {0}.itemId IN ({2}) {3};
)",
        table_name(table),
        join.transform([this, table](const auto& in) {
                return fmt::format(
                    "LEFT JOIN {1} ON {1}.itemId = {0}.itemId", table_name(table), table_name(in));
            })
            .value_or(""s),
        placeholders.join(", "),
        not_null.size() ? fmt::format("AND ({})", db::null<db::Logical::AND, db::Eq::NOT>(not_null))
                        : ""s));
    for (usize i = 0; i < ids.size(); ++i) {
        query.bindValue(placeholders[i], ids[i].toUrl());
    }
    if (query.exec()) {
        while (query.next()) {
            include.insert(model::ItemId { query.value(0).toUrl() });
        }
    }
    for (auto& el : ids) {
        if (! include.contains(el)) {
            out.emplace_back(el);
        }
    }
    co_return out;
}

} // namespace qcm