module;
#include <rstd/macro.hpp>
export module qcm.backend:db.state.favorite;
export import sqlito;
export import :db.error.query;
import liorm;
import liorm.sqlito;
import :db.schema;
import :db.timestamp;
import :db.codec;
import :model.dynamic;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace favorite_query = liorm::query;

namespace qcm::backend
{
inline constexpr auto favorite_upsert = favorite_query::compile([](favorite_query::QueryBuilder& b) {
    auto row = b.use(database_tables).table<Dynamic>();
    return b.insert_into(row.source(), row.col<&Dynamic::id>(), row.col<&Dynamic::favorite_at>())
        .values(b.param<i64>("id"_str),
                b.param<favorite_query::Nullable<Timestamp>>("favorite_at"_str))
        .on_conflict_do_update(
            array<favorite_query::Expr, 1> { row.col<&Dynamic::id>() }.as_slice(),
            array<favorite_query::Expr, 1> {
                row.col<&Dynamic::favorite_at>().set(b.excluded(row.col<&Dynamic::favorite_at>())) }
                .as_slice());
});
}

export namespace qcm::backend
{
auto write_favorite(sqlito::Connection& db, i64 id, bool value) -> QueryResult<empty> {
    auto stmt      = rstd_try(liorm::sqlito::prepare(db, favorite_upsert));
    auto timestamp = value ? Some(Timestamp::now()) : Option<Timestamp>(None());
    rstd_try(stmt.bind("id"_str, id));
    rstd_try(stmt.bind("favorite_at"_str, timestamp));
    rstd_try(liorm::sqlito::discard(stmt));
    return Ok(empty {});
}
} // namespace qcm::backend
