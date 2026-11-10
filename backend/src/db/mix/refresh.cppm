module;
#include <rstd/macro.hpp>
export module qcm.backend:db.mix.refresh;
export import sqlito;
export import :model.item;
export import :error;
import liorm;
import liorm.sqlito;
import :db.schema;
import :db.database;
import :db.error.query;
import :db.timestamp;
import :model.item_decode;
import :model.mix_model;
import :model.remote_mix;
template<class T>
using AsyncMutex = rstd::async::Mutex<T>;
using rstd::collections::HashMap;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using rstd::sync::Weak;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

export namespace qcm::backend
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

class MixRefresh {
    using Lock  = AsyncMutex<empty>;
    using Locks = HashMap<i64, Weak<Lock>>;
    Mutex<Locks> locks_ { Locks {} };

public:
    template<class Sync>
    auto refresh(sqlito::ExecutorHandle db, i64 mix_id, bool force, Sync sync)
        -> rstd::async::coro<ProcessResult<empty>> {
        auto lock = [&]() -> Arc<Lock> {
            auto     locks = locks_.lock().unwrap();
            Vec<i64> expired;
            for (auto entry : locks->iter())
                if (rstd::get<1>(entry)->strong_count() == usize())
                    expired.push(i64(*rstd::get<0>(entry)));
            for (auto id : expired) locks->remove(id);
            if (auto entry = locks->get(mix_id))
                if (auto live = (*entry)->upgrade()) return live;
            auto created = Arc<Lock>::make(empty {});
            locks->insert(mix_id, created.downgrade());
            return created;
        }();
        auto guard     = co_await lock->lock();
        auto candidate = rstd_co_try(
            co_await database_run(db.clone(), [mix_id, force](sqlito::Connection& connection) {
                return mix_refresh_candidate(connection, mix_id, force);
            }));
        if (candidate.is_none()) co_return Ok(empty {});
        co_return co_await sync(rstd::move(*candidate));
    }
};
} // namespace qcm::backend
