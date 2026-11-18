module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.artist;
import :db.codec;
import :db.schema;
import :model.artist;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
namespace
{
template<class Database>
auto artist_exists_impl(Database& db, i64 id) -> QueryResult<bool> {
    return Ok(rstd_try((liorm::sqlito::exists<database_tables, Artist>(db, id))));
}
} // namespace

auto artist_exists(sqlito::Connection& db, i64 id) -> QueryResult<bool> {
    return artist_exists_impl(db, id);
}

auto artist_exists(sqlito::Transaction& db, i64 id) -> QueryResult<bool> {
    return artist_exists_impl(db, id);
}
} // namespace qcm::backend
