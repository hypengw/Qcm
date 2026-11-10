module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.mix.remote_query;
export import :db.query.input;
export import :db.error.query;
export import sqlito;
import liorm;
import liorm.sqlito;
import :db.query.filter_expressions;
import :db.schema;
import :model.remote_mix;
import :model.mix_model;
import :pagination;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

export namespace qcm::backend
{
class RemoteMixFilterExpressions : public FilterExpressions {
public:
    using FilterExpressions::FilterExpressions;

    template<class RemoteTable>
    auto build(q::BoundSchema schema, const RemoteTable& remote,
               const QueryFilters<RemoteMixFilter>& filters) -> Option<q::Expr> {
        for (const auto& filter : filters.values) {
            Option<q::Expr> expr;
            RSTD_MATCH(filter.value) {
                RSTD_CASE(Name, f) {
                    expr = string(
                        remote.template col<&RemoteMix::name>(), f.value.as_str(), f.condition);
                }
                RSTD_CASE(TrackCount, f) {
                    expr = integer(
                        remote.template col<&RemoteMix::track_count>(), f.value, f.condition);
                }
                RSTD_CASE(Type, f) {
                    expr = type(
                        remote.template col<&RemoteMix::mix_type>(), f.value.as_str(), f.condition);
                }
                RSTD_CASE(LocalType, f) {
                    auto mix       = schema.template table<Mix>();
                    auto condition = type(mix.template col<&Mix::mix_type>(), f.value, f.condition);
                    if (condition) {
                        expr = Some(builder_.exists(
                            builder_.select(builder_.integer(1))
                                .from(mix.source())
                                .where(builder_.on(schema.relation(db_relations::mix_remote),
                                                   mix,
                                                   remote) &&
                                       *condition)));
                    }
                }
            }
            add(filter.group.to_primitive(), expr);
        }
        return grouped(filters.logics);
    }
};
struct RemoteMixQueryPage {
    Vec<i64> ids;
    i64      total {};
};
template<class Database>
auto remote_mix_query_page(Database& db, const RemoteMixQuery& request)
    -> QueryResult<RemoteMixQueryPage> {
    q::QueryBuilder            b;
    auto                       schema = b.use(database_tables);
    auto                       remote = schema.table<RemoteMix>();
    auto                       mix    = schema.table<Mix>();
    RemoteMixFilterExpressions filters(b);
    auto                       condition = filters.build(schema, remote, request.filters);
    auto                       statement =
        b.select(mix.col<&Mix::id>())
            .from(remote.source())
            .left_join(mix.source(), b.on(schema.relation(db_relations::mix_remote), mix, remote));
    if (condition) statement = statement.where(*condition);
    statement  = statement.order_by(remote.col<&RemoteMix::id>().asc());
    auto query = rstd_try(
        q::try_compile_page(statement, i64(request.page.size.to_primitive()), request.page.offset));
    auto stmt = rstd_try(liorm::sqlito::prepare_page(db, rstd::move(query)));
    rstd_try(filters.bind(stmt));
    auto               rows = rstd_try(stmt.fetch(liorm::TupleProjection<Option<i64>> {}));
    RemoteMixQueryPage result;
    result.total = rows.total;
    for (auto& row : rows.rows) {
        auto id = rstd::get<0>(row);
        if (id.is_some()) result.ids.push(rstd::move(*id));
    }
    return Ok(rstd::move(result));
}
} // namespace qcm::backend
