module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.mix.query;
export import :db.query.input;
export import :db.error.query;
export import sqlito;
import liorm;
import liorm.sqlito;
import :db.query.filter_expressions;
import :db.schema;
import :model.mix;
import :model.mix_model;
import :pagination;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

export namespace qcm::backend
{
class MixFilterExpressions : public FilterExpressions {
public:
    using FilterExpressions::FilterExpressions;

    template<class MixTable>
    auto build(const MixTable& mix, const QueryFilters<MixFilter>& filters) -> Option<q::Expr> {
        for (const auto& filter : filters.values) {
            Option<q::Expr> expr;
            RSTD_MATCH(filter.value) {
                RSTD_CASE(Name, f) {
                    expr = string(mix.template col<&Mix::name>(), f.value.as_str(), f.condition);
                }
                RSTD_CASE(TrackCount, f) {
                    expr = integer(mix.template col<&Mix::track_count>(), f.value, f.condition);
                }
                RSTD_CASE(Type, f) {
                    expr = type(mix.template col<&Mix::mix_type>(), f.value, f.condition);
                }
            }
            add(filter.group.to_primitive(), expr);
        }
        return grouped(filters.logics);
    }
};
template<class Database>
auto mix_query_page(Database& db, const MixQuery& request) -> QueryResult<MixIdPage> {
    q::QueryBuilder      b;
    auto                 schema = b.use(database_tables);
    auto                 mix    = schema.table<Mix>();
    MixFilterExpressions filters(b);
    auto                 condition = mix.col<&Mix::mix_type>() != b.integer(2);
    auto                 extra     = filters.build(mix, request.filters);
    if (extra) condition = condition && *extra;
    auto select = b.select(mix.col<&Mix::id>())
                      .from(mix.source())
                      .where(condition)
                      .order_by(mix.col<&Mix::id>().asc());
    auto query  = rstd_try(
        q::try_compile_page(select, i64(request.page.size.to_primitive()), request.page.offset));
    auto stmt = rstd_try(liorm::sqlito::prepare_page(db, rstd::move(query)));
    rstd_try(filters.bind(stmt));
    auto rows =
        rstd_try(stmt.fetch(QueryIdProjection { "mix_query returned a null ID or count"_str }));
    MixIdPage result;
    result.total = rows.total;
    result.ids   = rstd::move(rows.rows);
    return Ok(rstd::move(result));
}
} // namespace qcm::backend
