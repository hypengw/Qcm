module;
#include <rstd/macro.hpp>
module qcm.backend;
import :resource.sqlite;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::resource_cache
{
auto failure(ref<str> text) -> sqlito::Error {
    return sqlito::Error::from(sqlito::ErrorKind::State, 0, String::make(text));
}

auto binding_error(liorm::BindError<sqlito::Error> error) -> Error {
    if (error.writer_error.is_some()) return Error::storage(rstd::move(*error.writer_error));
    return Error::state("invalid resource query parameter"_str, rstd::move(error));
}

auto Database::raw_query(ref<str> sql) -> SqlResult<sqlito::Statement> {
    return transaction_.is_some() ? transaction_->prepare(sql) : connection_->prepare(sql);
}

auto Database::exec(ref<str> sql) -> SqlResult<empty> {
    return transaction_.is_some() ? transaction_->execute_batch(sql)
                                  : connection_->execute_batch(sql);
}

auto Database::open_blob(ref<str> table, ref<str> column, i64 row_id, sqlito::BlobMode mode)
    -> SqlResult<sqlito::Blob> {
    if (transaction_.is_none()) return Err(failure("cache blob requires a transaction"_str));
    return transaction_->open_blob("main"_str, table, column, row_id, mode);
}

auto Database::checkpoint() -> SqlResult<empty> {
    return exec("PRAGMA wal_checkpoint(PASSIVE)"_str);
}

auto Database::migrate() -> SqlResult<empty> {
    {
        auto row = rstd_try(raw_query("PRAGMA user_version"_str));
        (void)rstd_try(next(row));
        if (rstd_try(row.integer(usize())).unwrap_or(i64()) > i64(4))
            return Err(failure("unsupported resource cache schema"_str));
    }
    rstd_try(
        exec("PRAGMA journal_mode=WAL; PRAGMA synchronous=FULL; PRAGMA busy_timeout=5000;"_str));
    return atomic([&]() -> SqlResult<empty> {
        rstd_try(exec(
            "CREATE TABLE IF NOT EXISTS versions(id INTEGER PRIMARY KEY,resource_key TEXT NOT "
            "NULL,provider INTEGER NOT NULL,kind INTEGER NOT NULL,generation TEXT NOT NULL "
            "UNIQUE,meta TEXT NOT NULL,blocks BLOB NOT NULL,last_use INTEGER NOT NULL,obsolete "
            "INTEGER NOT NULL DEFAULT 0); CREATE INDEX IF NOT EXISTS resource_lookup ON "
            "versions(resource_key,id);"
            "CREATE TABLE IF NOT EXISTS complete_files(version_id INTEGER PRIMARY KEY,ready INTEGER NOT NULL DEFAULT 0,length INTEGER NOT NULL DEFAULT 0,digest BLOB,retry_at INTEGER NOT NULL DEFAULT 0,format INTEGER NOT NULL DEFAULT 1); CREATE TABLE IF NOT EXISTS file_leases(token TEXT PRIMARY KEY,version_id INTEGER NOT NULL,client TEXT NOT NULL,expires INTEGER NOT NULL); CREATE INDEX IF NOT EXISTS lease_versions ON file_leases(version_id); CREATE TABLE IF NOT EXISTS file_clock(id INTEGER PRIMARY KEY CHECK(id=1),time INTEGER NOT NULL);"_str));
        bool has_format = false;
        {
            auto row = rstd_try(raw_query("PRAGMA table_info(complete_files)"_str));
            while (rstd_try(next(row))) {
                auto name = rstd_try(row.text(usize(1)));
                if (name.is_none()) return Err(failure("missing cache column name"_str));
                has_format |= name->as_str() == "format"_str;
            }
        }
        if (! has_format)
            rstd_try(exec(
                "ALTER TABLE complete_files ADD COLUMN format INTEGER NOT NULL DEFAULT 1"_str));
        return exec(
            "CREATE TABLE IF NOT EXISTS segments(version_id INTEGER NOT NULL,number INTEGER NOT NULL,token TEXT NOT NULL,length INTEGER NOT NULL,last_use INTEGER NOT NULL,deleting INTEGER NOT NULL DEFAULT 0,PRIMARY KEY(version_id,number)); CREATE INDEX IF NOT EXISTS segment_lru ON segments(deleting,last_use); CREATE TABLE IF NOT EXISTS packs(token TEXT PRIMARY KEY,group_key TEXT NOT NULL,provider INTEGER NOT NULL,length INTEGER NOT NULL,last_use INTEGER NOT NULL,deleting INTEGER NOT NULL DEFAULT 0); CREATE INDEX IF NOT EXISTS pack_group ON packs(group_key,deleting,last_use); CREATE TABLE IF NOT EXISTS objects(resource_key TEXT PRIMARY KEY,pack TEXT NOT NULL,offset INTEGER NOT NULL,length INTEGER NOT NULL,digest BLOB NOT NULL,meta TEXT NOT NULL); CREATE INDEX IF NOT EXISTS object_pack ON objects(pack); PRAGMA user_version=4;"_str);
    });
}
} // namespace qcm::backend::resource_cache
