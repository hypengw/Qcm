module;
#include <rstd/macro.hpp>
export module qcm.backend:db.provider.status;
export import :model.library;
export import :db.error.query;
import :db.codec;
import :db.schema;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace provider_status_query = liorm::query;

namespace qcm::backend::provider_status_detail
{
inline constexpr provider_status_query::ProjectionLayout<provider_status_query::ModelGroup<Library>>
                      libraries;
inline constexpr auto all =
    provider_status_query::compile([](provider_status_query::QueryBuilder& b) {
        auto table = b.use(database_tables).table<Library>();
        return b.select(provider_status_query::project(libraries, table).as_slice())
            .from(table.source());
    });
} // namespace qcm::backend::provider_status_detail

export namespace qcm::backend
{
auto provider_status_libraries(sqlito::Connection& db) -> QueryResult<Vec<Library>> {
    return Ok(rstd_try(liorm::sqlito::fetch_all<provider_status_detail::all>(
        db, liorm::ModelProjection<Library> {})));
}
} // namespace qcm::backend
