module;
#include <rstd/macro.hpp>
export module qcm.backend:db.size;
export import rstd;
export import sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
auto pragma_integer(sqlito::Connection& db, ref<str> sql) -> i64 {
    auto stmt = db.prepare(sql);
    if (stmt.is_err()) return i64();
    auto prepared = rstd::move(stmt).unwrap();
    auto step     = prepared.step();
    if (step.is_err() || *step != sqlito::Step::Row) return i64();
    auto value = prepared.integer(usize());
    if (value.is_err()) return i64();
    return value->unwrap_or(i64());
}
auto sqlite_disk_bytes(sqlito::Connection& db) -> i64 {
    return pragma_integer(db, "PRAGMA page_count"_str) * pragma_integer(db, "PRAGMA page_size"_str);
}
} // namespace qcm::backend
