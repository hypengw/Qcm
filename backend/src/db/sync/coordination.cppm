module;
#include <rstd/macro.hpp>
export module qcm.backend:db.sync.coordination;
export import :db.sync.run;
import :db.schema;
import :model.item;
import :model.mix_model;
import liorm.sqlito;
using rstd::collections::BTreeSet;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace write_query = liorm::query;

namespace qcm::backend
{
inline constexpr auto write_item_owner = write_query::compile([](write_query::QueryBuilder& b) {
    auto item = b.use(database_tables).table<Item>();
    return b.select(item.col<&Item::provider_id>())
        .from(item.source())
        .where(item.col<&Item::id>() == b.param<i64>("id"_str));
});
inline constexpr auto write_mix_remote = write_query::compile([](write_query::QueryBuilder& b) {
    auto mix = b.use(database_tables).table<Mix>();
    return b.select(mix.col<&Mix::remote_id>())
        .from(mix.source())
        .where(mix.col<&Mix::id>() == b.param<i64>("id"_str));
});
auto                  write_bind(Result<empty, liorm::BindError<sqlito::Error>> result)
    -> Result<empty, sqlito::Error> {
    if (result.is_ok()) return Ok(empty {});
    auto error = rstd::move(result).unwrap_err();
    if (error.writer_error.is_some()) return Err(rstd::move(error.writer_error).unwrap());
    return Err(sync_guard_error("write coordination bind failed"_str));
}
} // namespace qcm::backend

export namespace qcm::backend
{
using ProviderWriteGuards = Vec<Box<ProviderWriteGuard>>;

auto write_items_locked(SyncCoordinator& coordinator, sqlito::Connection& db, slice<i64> ids,
                        ProjectionWriteLock projection)
    -> Result<ProviderWriteGuards, sqlito::Error> {
    BTreeSet<i64> providers;
    providers.insert(i64());
    auto query = rstd_try(liorm::sqlito::prepare(db, write_item_owner));
    for (auto id : ids) {
        rstd_try(write_bind(query.bind("id"_str, id)));
        auto provider = rstd_try(liorm::sqlito::optional_scalar<i64>(
            query, usize(), liorm::sqlito::RowConsumption::First));
        if (provider.is_some()) providers.insert(*provider);
        rstd_try(query.reset());
    }
    ProviderWriteGuards guards;
    for (auto provider : providers.iter()) {
        auto guard = rstd_try(coordinator.write(i64(*provider), projection.clone()));
        guards.push(rstd::move(guard));
    }
    return Ok(rstd::move(guards));
}
auto write_items(SyncCoordinator& coordinator, sqlito::Connection& db, slice<i64> ids)
    -> Result<ProviderWriteGuards, sqlito::Error> {
    auto projection = coordinator.lock_writes();
    return write_items_locked(coordinator, db, ids, rstd::move(projection));
}
auto write_mixes(SyncCoordinator& coordinator, sqlito::Connection& db, slice<i64> mix_ids,
                 slice<i64> item_ids) -> Result<ProviderWriteGuards, sqlito::Error> {
    auto     projection = coordinator.lock_writes();
    Vec<i64> affected;
    for (auto id : item_ids) affected.push(i64(id));
    auto query = rstd_try(liorm::sqlito::prepare(db, write_mix_remote));
    for (auto id : mix_ids) {
        rstd_try(write_bind(query.bind("id"_str, id)));
        auto remote = rstd_try(liorm::sqlito::optional_scalar<Option<i64>>(
            query, usize(), liorm::sqlito::RowConsumption::First));
        if (remote.is_some() && remote->is_some()) affected.push(i64(**remote));
        rstd_try(query.reset());
    }
    return write_items_locked(coordinator, db, affected.as_slice(), rstd::move(projection));
}
} // namespace qcm::backend
