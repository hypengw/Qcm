module;
#include <rstd/macro.hpp>
export module qcm.backend:db.mix.refresh;
export import sqlito;
export import :model.item;
export import :error;
import :db.database;
import :db.error.query;
template<class T>
using AsyncMutex = rstd::async::Mutex<T>;
using rstd::collections::HashMap;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using rstd::sync::Weak;
using namespace rstd::prelude;

export namespace qcm::backend
{
auto mix_refresh_candidate(sqlito::Connection& db, i64 mix_id, bool force)
    -> QueryResult<Option<Item>>;

class MixRefresh {
    using Lock  = AsyncMutex<empty>;
    using Locks = HashMap<i64, Weak<Lock>>;
    Mutex<Locks> locks_ { Locks {} };

    auto lock_for(i64 mix_id) -> Arc<Lock>;

public:
    template<class Sync>
    auto refresh(sqlito::ExecutorHandle db, i64 mix_id, bool force, Sync sync)
        -> rstd::async::coro<ProcessResult<empty>> {
        auto lock      = lock_for(mix_id);
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
