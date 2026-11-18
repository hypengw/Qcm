export module qcm.backend:api.network;
export import :protocol.message;
export import :error;
export import rstd;
export import sqlito;
export import :network;
using namespace rstd::prelude;
using QcmMessage = p_qcm::p_msg::M_QcmMessage;

export namespace qcm::backend
{
auto restore_network_settings(sqlito::Connection& db) -> ProcessResult<empty>;

auto restore_network_settings(sqlito::ExecutorHandle db) -> rstd::async::coro<ProcessResult<empty>>;

auto get_network_settings(sqlito::Connection& db, i32 request_id) -> ProcessResult<QcmMessage>;

auto update_network_settings(sqlito::Connection& db, i32 request_id,
                             const p_qcm::p_msg::M_UpdateNetworkSettingsReq& entry)
    -> ProcessResult<QcmMessage>;
} // namespace qcm::backend
