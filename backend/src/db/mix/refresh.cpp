module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.mix.refresh;
import :model.item_decode;
import :db.codec;
import :db.schema;
import :model.mix_model;
import :model.remote_mix;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::sync::Arc;
namespace q = liorm::query;

namespace qcm::backend
{
namespace
{
inline constexpr q::ProjectionLayout<q::ModelGroup<Item>> mix_refresh_item_projection;
template<bool Force>
inline constexpr auto mix_refresh_candidate_query = q::compile([](q::QueryBuilder& b) {
    auto schema    = b.use(database_tables);
    auto mix       = schema.table<Mix>();
    auto remote    = schema.table<RemoteMix>();
    auto item      = schema.table<Item>();
    auto condition = mix.col<&Mix::id>() == b.param<i64>("mix_id"_str);
    if constexpr (! Force)
        condition =
            condition && (mix.col<&Mix::content_update_at>() <= b.param<Timestamp>("cutoff"_str));
    return b.select(q::project(mix_refresh_item_projection, item).as_slice())
        .from(mix.source())
        .join(remote.source(), mix.col<&Mix::remote_id>() == remote.col<&RemoteMix::id>())
        .join(item.source(), remote.col<&RemoteMix::id>() == item.col<&Item::id>())
        .where(condition)
        .limit(b.integer(1));
});
} // namespace

auto mix_refresh_candidate(sqlito::Connection& db, i64 mix_id, bool force)
    -> QueryResult<Option<Item>> {
    if (force)
        return Ok(rstd_try(liorm::sqlito::fetch_model<mix_refresh_candidate_query<true>>(
            db,
            mix_refresh_item_projection.group<0>(),
            liorm::parameters(liorm::param<"mix_id">(mix_id)))));
    return Ok(rstd_try(liorm::sqlito::fetch_model<mix_refresh_candidate_query<false>>(
        db,
        mix_refresh_item_projection.group<0>(),
        liorm::parameters(liorm::param<"mix_id">(mix_id),
                          liorm::param<"cutoff">(Timestamp::from_millis(
                              Timestamp::now().as_millis() - i64(30 * 60 * 1000)))))));
}

auto MixRefresh::lock_for(i64 mix_id) -> Arc<Lock> {
    auto     locks = locks_.lock().unwrap();
    Vec<i64> expired;
    for (auto entry : locks->iter())
        if (rstd::get<1>(entry)->strong_count() == usize()) expired.push(i64(*rstd::get<0>(entry)));
    for (auto id : expired) locks->remove(id);
    if (auto entry = locks->get(mix_id))
        if (auto live = (*entry)->upgrade()) return live;
    auto created = Arc<Lock>::make(empty {});
    locks->insert(mix_id, created.downgrade());
    return created;
}
} // namespace qcm::backend
