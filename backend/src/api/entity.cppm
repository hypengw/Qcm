module;
#include <rstd/macro.hpp>
export module qcm.backend:api.entity;
import :db.database;
import :db.sync.coordination;
import :entity;
import :error;
import :event;
using rstd::async::coro;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
class EntityQueries {
    sqlito::ExecutorHandle database_;
    SyncCoordinator        coordinator_;

    static auto populated(QcmMessage response) -> coro<ProcessResult<QcmMessage>> {
        co_return Ok(rstd::move(response));
    }

public:
    EntityQueries(sqlito::ExecutorHandle database, SyncCoordinator coordinator)
        : database_(rstd::move(database)), coordinator_(rstd::move(coordinator)) {}

    auto populate(QcmMessage response, Option<Arc<QcmMessage>> request = None()) const
        -> coro<ProcessResult<QcmMessage>> {
        if (response.f_entities.is_some()) return populated(rstd::move(response));
        return database_run(database_.clone(),
                            [response    = rstd::move(response),
                             request     = rstd::move(request),
                             coordinator = coordinator_.clone()](
                                sqlito::Connection& database) mutable -> ProcessResult<QcmMessage> {
                                auto projection     = coordinator.lock_snapshot();
                                auto transaction    = rstd_try(database.begin());
                                response.f_entities = Some(rstd_try(
                                    qcm_entity_batch(transaction,
                                                     projection.snapshot,
                                                     response,
                                                     request.is_some() ? &**request : nullptr)));
                                rstd_try(transaction.commit());
                                return Ok(rstd::move(response));
                            });
    }

    template<class Query>
    auto query(Query query) const -> coro<ProcessResult<Option<QcmMessage>>> {
        return database_run(
            database_.clone(),
            [coordinator = coordinator_.clone(), query = rstd::move(query)](
                sqlito::Connection& database) mutable -> ProcessResult<Option<QcmMessage>> {
                auto projection  = coordinator.lock_snapshot();
                auto transaction = rstd_try(database.begin());
                auto response    = rstd_try(query(transaction));
                response.f_entities =
                    Some(rstd_try(qcm_entity_batch(transaction, projection.snapshot, response)));
                rstd_try(transaction.commit());
                return Ok(Some(rstd::move(response)));
            });
    }
};
} // namespace qcm::backend
