module;
#include <rstd/macro.hpp>
export module qcm.backend:db.migrate;
export import rstd;
export import sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct Migration {
    ref<str>        version;
    slice<ref<str>> statements;
};
struct MigrationReport {
    usize applied {};
    usize skipped {};
};

namespace migrate_detail
{
constexpr auto now_ms = "(strftime('%s','now')*1000)"_str;

constexpr auto provider =
    array<ref<str>, 1> { "CREATE TABLE IF NOT EXISTS provider("
                         "provider_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                         "name TEXT NOT NULL,"
                         "type TEXT NOT NULL,"
                         "base_url TEXT NOT NULL,"
                         "auth_method JSON,"
                         "cookie TEXT NOT NULL,"
                         "custom TEXT NOT NULL,"
                         "edit_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)"_str };
constexpr auto library = array<ref<str>, 2> {
    "CREATE TABLE IF NOT EXISTS library("
    "library_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "name TEXT NOT NULL,"
    "provider_id BIGINT NOT NULL,"
    "native_id TEXT NOT NULL,"
    "edit_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
    "FOREIGN KEY(provider_id) REFERENCES provider(provider_id) ON DELETE CASCADE)"_str,
    "CREATE UNIQUE INDEX IF NOT EXISTS library_provider_id_native_id "
    "ON library(provider_id, native_id)"_str
};
constexpr auto main_tables = array<ref<str>, 17> {
    "CREATE TABLE IF NOT EXISTS item("
    "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "native_id TEXT NOT NULL,"
    "library_id BIGINT,"
    "provider_id BIGINT NOT NULL,"
    "type INTEGER NOT NULL,"
    "create_at BIGINT NOT NULL DEFAULT (strftime('%s','now')*1000),"
    "update_at BIGINT NOT NULL DEFAULT (strftime('%s','now')*1000),"
    "last_sync_at BIGINT NOT NULL DEFAULT (strftime('%s','now')*1000),"
    "FOREIGN KEY(library_id) REFERENCES library(library_id) ON DELETE CASCADE)"_str,
    "CREATE INDEX IF NOT EXISTS item_native_id ON item(native_id)"_str,
    "CREATE UNIQUE INDEX IF NOT EXISTS item_native_type_library "
    "ON item(native_id, type, library_id) WHERE library_id IS NOT NULL"_str,
    "CREATE UNIQUE INDEX IF NOT EXISTS item_native_type_provider "
    "ON item(native_id, type, provider_id, library_id)"_str,
    "CREATE TABLE IF NOT EXISTS album("
    "id INTEGER PRIMARY KEY NOT NULL,"
    "name TEXT NOT NULL,"
    "sort_name TEXT,"
    "track_count INTEGER NOT NULL,"
    "disc_count INTEGER NOT NULL,"
    "type INTEGER DEFAULT 0,"
    "duration INTEGER NOT NULL,"
    "description TEXT,"
    "company TEXT,"
    "language TEXT,"
    "publish_time BIGINT,"
    "added_at BIGINT,"
    "FOREIGN KEY(id) REFERENCES item(id) ON DELETE CASCADE)"_str,
    "CREATE TABLE IF NOT EXISTS artist("
    "id INTEGER PRIMARY KEY NOT NULL,"
    "name TEXT NOT NULL,"
    "sort_name TEXT,"
    "description TEXT NOT NULL,"
    "album_count INTEGER NOT NULL,"
    "music_count INTEGER NOT NULL,"
    "added_at BIGINT,"
    "FOREIGN KEY(id) REFERENCES item(id) ON DELETE CASCADE)"_str,
    "CREATE TABLE IF NOT EXISTS song("
    "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "name TEXT NOT NULL,"
    "sort_name TEXT,"
    "album_id BIGINT,"
    "track_number INTEGER NOT NULL,"
    "disc_number INTEGER NOT NULL,"
    "duration BIGINT NOT NULL,"
    "can_play BOOLEAN NOT NULL,"
    "tags JSON NOT NULL,"
    "popularity DOUBLE NOT NULL,"
    "publish_time BIGINT,"
    "added_at BIGINT,"
    "FOREIGN KEY(id) REFERENCES item(id) ON DELETE CASCADE,"
    "FOREIGN KEY(album_id) REFERENCES album(id) ON DELETE CASCADE)"_str,
    "CREATE TABLE IF NOT EXISTS rel_album_artist("
    "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "album_id BIGINT NOT NULL,"
    "artist_id BIGINT NOT NULL,"
    "update_at BIGINT NOT NULL DEFAULT (strftime('%s','now')*1000),"
    "FOREIGN KEY(album_id) REFERENCES album(id) ON DELETE CASCADE,"
    "FOREIGN KEY(artist_id) REFERENCES artist(id) ON DELETE CASCADE)"_str,
    "CREATE UNIQUE INDEX IF NOT EXISTS rel_album_artist_album_id_artist_id "
    "ON rel_album_artist(album_id, artist_id)"_str,
    "CREATE INDEX IF NOT EXISTS rel_album_artist_album_id ON rel_album_artist(album_id)"_str,
    "CREATE INDEX IF NOT EXISTS rel_album_artist_artist_id ON rel_album_artist(artist_id)"_str,
    "CREATE TABLE IF NOT EXISTS rel_song_artist("
    "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "song_id BIGINT NOT NULL,"
    "artist_id BIGINT NOT NULL,"
    "update_at BIGINT NOT NULL DEFAULT (strftime('%s','now')*1000),"
    "FOREIGN KEY(song_id) REFERENCES song(id) ON DELETE CASCADE,"
    "FOREIGN KEY(artist_id) REFERENCES artist(id) ON DELETE CASCADE)"_str,
    "CREATE UNIQUE INDEX IF NOT EXISTS rel_song_artist_song_id_artist_id "
    "ON rel_song_artist(song_id, artist_id)"_str,
    "CREATE INDEX IF NOT EXISTS rel_song_artist_song_id ON rel_song_artist(song_id)"_str,
    "CREATE INDEX IF NOT EXISTS rel_song_artist_artist_id ON rel_song_artist(artist_id)"_str,
    "CREATE TABLE IF NOT EXISTS dynamic("
    "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "play_count BIGINT NOT NULL DEFAULT 0,"
    "remote_play_count BIGINT NOT NULL DEFAULT 0,"
    "is_external BOOLEAN NOT NULL DEFAULT 0,"
    "last_played_at BIGINT,"
    "remote_last_played_at BIGINT,"
    "favorite_at BIGINT,"
    "last_position BIGINT,"
    "update_at BIGINT NOT NULL DEFAULT (strftime('%s','now')*1000),"
    "FOREIGN KEY(id) REFERENCES item(id) ON DELETE CASCADE ON UPDATE CASCADE)"_str,
    "CREATE TABLE IF NOT EXISTS image("
    "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "item_id BIGINT NOT NULL,"
    "image_type TEXT NOT NULL,"
    "native_id TEXT,"
    "db TEXT,"
    "fresh TEXT NOT NULL,"
    "timestamp TIMESTAMP NOT NULL,"
    "FOREIGN KEY(item_id) REFERENCES item(id) ON DELETE CASCADE ON UPDATE CASCADE,"
    "UNIQUE(item_id, image_type))"_str
};
constexpr auto remote_mix = array<ref<str>, 6> {
    "CREATE UNIQUE INDEX IF NOT EXISTS item_native_type_provider_library_idx "
    "ON item(native_id, type, provider_id, COALESCE(library_id, -1))"_str,
    "CREATE TABLE IF NOT EXISTS remote_mix("
    "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "name TEXT NOT NULL,"
    "description TEXT,"
    "mix_type TEXT,"
    "track_count INTEGER,"
    "FOREIGN KEY(id) REFERENCES item(id) ON DELETE CASCADE)"_str,
    "CREATE TABLE IF NOT EXISTS mix("
    "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "name TEXT NOT NULL,"
    "sort_name TEXT,"
    "mix_type INTEGER NOT NULL,"
    "remote_id BIGINT,"
    "track_count INTEGER NOT NULL,"
    "description TEXT NOT NULL,"
    "content_update_at BIGINT NOT NULL DEFAULT 0,"
    "create_at BIGINT NOT NULL DEFAULT (strftime('%s','now')*1000),"
    "update_at BIGINT NOT NULL DEFAULT (strftime('%s','now')*1000),"
    "FOREIGN KEY(remote_id) REFERENCES remote_mix(id) ON DELETE CASCADE)"_str,
    "CREATE UNIQUE INDEX IF NOT EXISTS idx_mix_remote_id ON mix(remote_id)"_str,
    "CREATE TABLE IF NOT EXISTS rel_mix_song("
    "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "song_id BIGINT NOT NULL,"
    "mix_id BIGINT NOT NULL,"
    "order_idx BIGINT NOT NULL,"
    "update_at BIGINT NOT NULL DEFAULT (strftime('%s','now')*1000),"
    "FOREIGN KEY(song_id) REFERENCES song(id) ON DELETE CASCADE,"
    "FOREIGN KEY(mix_id) REFERENCES mix(id) ON DELETE CASCADE)"_str,
    "CREATE UNIQUE INDEX IF NOT EXISTS rel_mix_song_mix_id_song_id "
    "ON rel_mix_song(mix_id, song_id)"_str
};
constexpr auto fts = array<ref<str>, 16> {
    "DROP TABLE IF EXISTS album_fts"_str,
    "DROP TABLE IF EXISTS artist_fts"_str,
    "DROP TABLE IF EXISTS song_fts"_str,
    "DROP TABLE IF EXISTS mix_fts"_str,
    "CREATE VIRTUAL TABLE IF NOT EXISTS album_fts USING fts5 (name,description,"
    "content='album',content_rowid='id',tokenize='qcm')"_str,
    "CREATE TRIGGER album_fts_i AFTER INSERT ON album BEGIN "
    "INSERT INTO album_fts(rowid, name, description) VALUES (new.id, new.name, new.description); "
    "END"_str,
    "CREATE TRIGGER album_fts_d AFTER DELETE ON album BEGIN "
    "INSERT INTO album_fts(album_fts, rowid, name, description) "
    "VALUES('delete', old.id, old.name, old.description); END"_str,
    "CREATE TRIGGER album_fts_u AFTER UPDATE ON album BEGIN "
    "INSERT INTO album_fts(album_fts, rowid, name, description) "
    "VALUES('delete', old.id, old.name, old.description); "
    "INSERT INTO album_fts(rowid, name, description) VALUES (new.id, new.name, new.description); "
    "END"_str,
    "CREATE VIRTUAL TABLE IF NOT EXISTS artist_fts USING fts5 (name,description,"
    "content='artist',content_rowid='id',tokenize='qcm')"_str,
    "CREATE TRIGGER artist_fts_i AFTER INSERT ON artist BEGIN "
    "INSERT INTO artist_fts(rowid, name, description) VALUES (new.id, new.name, "
    "new.description); END"_str,
    "CREATE VIRTUAL TABLE IF NOT EXISTS mix_fts USING fts5 (name,description,"
    "content='mix',content_rowid='id',tokenize='qcm')"_str,
    "CREATE TRIGGER mix_fts_i AFTER INSERT ON mix BEGIN "
    "INSERT INTO mix_fts(rowid, name, description) VALUES (new.id, new.name, new.description); "
    "END"_str,
    "CREATE VIRTUAL TABLE IF NOT EXISTS song_fts USING fts5 (name,"
    "content='song',content_rowid='id',tokenize='qcm')"_str,
    "CREATE TRIGGER song_fts_i AFTER INSERT ON song BEGIN "
    "INSERT INTO song_fts(rowid, name) VALUES (new.id, new.name); END"_str,
    "CREATE TRIGGER song_fts_d AFTER DELETE ON song BEGIN "
    "INSERT INTO song_fts(song_fts, rowid, name) VALUES('delete', old.id, old.name); END"_str,
    "CREATE TRIGGER song_fts_u AFTER UPDATE ON song BEGIN "
    "INSERT INTO song_fts(song_fts, rowid, name) VALUES('delete', old.id, old.name); "
    "INSERT INTO song_fts(rowid, name) VALUES (new.id, new.name); END"_str
};
constexpr auto image_integer = array<ref<str>, 2> {
    "CREATE TABLE image_integer ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
    "item_id BIGINT NOT NULL REFERENCES item(id) ON DELETE CASCADE ON UPDATE CASCADE,"
    "image_type INTEGER NOT NULL,"
    "native_id TEXT, db TEXT, fresh TEXT NOT NULL, timestamp TIMESTAMP NOT NULL,"
    "UNIQUE(item_id,image_type))"_str,
    "INSERT INTO image_integer SELECT id,item_id,"
    "CASE image_type WHEN 'Primary' THEN 0 WHEN 'Backdrop' THEN 1 WHEN 'Banner' THEN 2 "
    "WHEN 'Thumb' THEN 3 WHEN 'Logo' THEN 4 ELSE CAST(image_type AS INTEGER) END,"
    "native_id,db,fresh,timestamp FROM image;"
    "DROP TABLE image;"
    "ALTER TABLE image_integer RENAME TO image;"_str
};
constexpr auto provider_session = array<ref<str>, 1> {
    "ALTER TABLE provider ADD COLUMN session_key TEXT NOT NULL DEFAULT ''"_str
};
constexpr auto provider_settings = array<ref<str>, 1> {
    "CREATE TABLE provider_settings ("
    "provider_id BIGINT PRIMARY KEY NOT NULL REFERENCES provider(provider_id) ON DELETE CASCADE,"
    "revision BIGINT NOT NULL, config JSON NOT NULL)"_str
};
constexpr auto network_proxies = array<ref<str>, 4> {
    "CREATE TABLE network_settings(id INTEGER PRIMARY KEY CHECK(id=1), revision BIGINT NOT NULL)"_str,
    "INSERT INTO network_settings(id,revision) VALUES(1,0)"_str,
    "CREATE TABLE network_proxy(proxy_id TEXT PRIMARY KEY NOT NULL,name TEXT NOT NULL,"
    "url TEXT NOT NULL,position INTEGER NOT NULL)"_str,
    "CREATE TABLE provider_proxy(provider_id BIGINT PRIMARY KEY NOT NULL "
    "REFERENCES provider(provider_id) ON DELETE CASCADE,"
    "proxy_id TEXT NOT NULL REFERENCES network_proxy(proxy_id) ON DELETE CASCADE)"_str
};
constexpr auto plugin_state = array<ref<str>, 1> { "CREATE TABLE plugin_installation_state ("
                                                   "namespace TEXT PRIMARY KEY NOT NULL,"
                                                   "revision BIGINT NOT NULL CHECK(revision > 0),"
                                                   "data TEXT NOT NULL)"_str };
constexpr auto cache_tables =
    array<ref<str>, 2> { "CREATE TABLE IF NOT EXISTS cache("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                         "key TEXT NOT NULL,"
                         "cache_type INTEGER NOT NULL,"
                         "content_type TEXT NOT NULL,"
                         "content_length BIGINT UNSIGNED NOT NULL,"
                         "blob BLOB,"
                         "timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                         "last_use TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)"_str,
                         "CREATE UNIQUE INDEX IF NOT EXISTS cache_key ON cache(key)"_str };
constexpr auto cache_blocks =
    array<ref<str>, 3> { "CREATE TABLE IF NOT EXISTS cache_source("
                         "key TEXT PRIMARY KEY NOT NULL,"
                         "cache_type INTEGER NOT NULL,"
                         "content_type TEXT NOT NULL,"
                         "content_length BIGINT NOT NULL,"
                         "block_count INTEGER NOT NULL,"
                         "timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                         "last_use TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)"_str,
                         "CREATE TABLE IF NOT EXISTS cache_block("
                         "key TEXT PRIMARY KEY NOT NULL,"
                         "source_key TEXT NOT NULL,"
                         "block_index INTEGER NOT NULL,"
                         "block_size BIGINT NOT NULL,"
                         "blob BLOB,"
                         "timestamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)"_str,
                         "CREATE UNIQUE INDEX IF NOT EXISTS cache_block_source_key_block_index "
                         "ON cache_block(source_key, block_index)"_str };

auto table() -> array<Migration, 10> {
    return array<Migration, 10> {
        Migration { "m20220101_000004_create_provider_table"_str, provider.as_slice() },
        Migration { "m20220101_000005_create_library_table"_str, library.as_slice() },
        Migration { "m20250824_000001_create_table"_str, main_tables.as_slice() },
        Migration { "m20251129_145233_create_remote_mix"_str, remote_mix.as_slice() },
        Migration { "m20251214_145233_create_fts_table"_str, fts.as_slice() },
        Migration { "m20261007_000001_image_type_integer"_str, image_integer.as_slice() },
        Migration { "m20261017_000001_provider_session"_str, provider_session.as_slice() },
        Migration { "m20261019_000001_provider_settings"_str, provider_settings.as_slice() },
        Migration { "m20261019_000002_network_proxies"_str, network_proxies.as_slice() },
        Migration { "m20261104_000001_plugin_installation_state"_str, plugin_state.as_slice() },
    };
}
auto cache_table() -> array<Migration, 2> {
    return array<Migration, 2> {
        Migration { "m20220101_000001_create_table"_str, cache_tables.as_slice() },
        Migration { "m20260404_000001_create_block_cache"_str, cache_blocks.as_slice() },
    };
}
auto installed(sqlito::Connection& db, ref<str> version) -> Result<bool, sqlito::Error> {
    auto stmt = rstd_try(db.prepare("SELECT 1 FROM seaql_migrations WHERE version = ?1"_str));
    rstd_try(stmt.bind_text(usize(1), version));
    return Ok(rstd_try(stmt.step()) == sqlito::Step::Row);
}
auto apply(sqlito::Connection& db, const Migration& migration) -> Result<empty, sqlito::Error> {
    auto begun = rstd_try(db.begin());
    auto tx    = rstd::move(begun);
    for (const auto& statement : migration.statements) rstd_try(tx.execute_batch(statement));
    auto ledger = rstd_try(tx.prepare("INSERT INTO seaql_migrations(version, applied_at) "
                                      "VALUES(?1, CAST(strftime('%s','now') AS INTEGER))"_str));
    rstd_try(ledger.bind_text(usize(1), migration.version));
    rstd_try(ledger.step());
    rstd_try(tx.commit());
    return Ok(empty {});
}
auto run(sqlito::Connection& db, slice<Migration> migrations)
    -> Result<MigrationReport, sqlito::Error> {
    rstd_try(db.execute_batch("CREATE TABLE IF NOT EXISTS seaql_migrations("
                              "version TEXT PRIMARY KEY NOT NULL,"
                              "applied_at INTEGER NOT NULL)"_str));
    MigrationReport report;
    for (const auto& migration : migrations) {
        if (rstd_try(installed(db, migration.version))) {
            report.skipped = report.skipped + usize(1);
            continue;
        }
        rstd_try(apply(db, migration));
        report.applied = report.applied + usize(1);
    }
    return Ok(report);
}
} // namespace migrate_detail

auto qcm_migrations() -> slice<Migration> {
    static const auto migrations = migrate_detail::table();
    return migrations.as_slice();
}
auto qcm_cache_migrations() -> slice<Migration> {
    static const auto migrations = migrate_detail::cache_table();
    return migrations.as_slice();
}
auto migrate(sqlito::Connection& db, slice<Migration> migrations)
    -> Result<MigrationReport, sqlito::Error> {
    // The connection owner keeps its FTS registrations alive through migration and later writes.
    return migrate_detail::run(db, migrations);
}
auto migrate(sqlito::Connection& db) -> Result<MigrationReport, sqlito::Error> {
    return migrate(db, qcm_migrations());
}
auto migrate_cache(sqlito::Connection& db) -> Result<MigrationReport, sqlito::Error> {
    return migrate_detail::run(db, qcm_cache_migrations());
}
} // namespace qcm::backend
