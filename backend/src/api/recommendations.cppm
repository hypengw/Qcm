export module qcm.backend:api.recommendations;
import :protocol.message;
import :provider.runtime;
import :provider.item;
import :db.sync.run;
import :model.home;
import :error;
import sqlito;
import rstd;
using namespace rstd::prelude;

export namespace qcm::backend
{
auto build_home_cards(sqlito::Connection& db, SyncCoordinator& coordinator, i64 provider,
                      Vec<HomeEntry>&                                entries,
                      Vec<Box<p_qcm::p_msg::p_model::M_RadioQueue>>& radios,
                      p_qcm::p_msg::p_model::M_HomeRecommendation& output) -> ProcessResult<empty>;
auto get_home_recommendations_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                                    SyncCoordinator& coordinator, i32 request_id,
                                    const p_qcm::p_msg::M_GetHomeRecommendationsReq& entry,
                                    ProviderRequestOwnerHandle                       owner = {})
    -> rstd::async::coro<ProcessResult<p_qcm::p_msg::M_QcmMessage>>;
} // namespace qcm::backend
