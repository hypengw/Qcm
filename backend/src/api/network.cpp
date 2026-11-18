module;
#include <rstd/macro.hpp>
module qcm.backend;
import :api.network;
import :db.database;
import :db.state.network_settings;
import rstd.json;
using namespace rstd::prelude;
using namespace rstd::literals;
using QcmMessage = p_qcm::p_msg::M_QcmMessage;

namespace qcm::backend::network_api
{
auto ok_rsp(i32 id) -> QcmMessage { return qcm_message(id, Box<p_qcm::p_msg::M_Rsp>::make()); }
} // namespace qcm::backend::network_api

namespace qcm::backend
{
auto restore_network_settings(sqlito::Connection& db) -> ProcessResult<empty> {
    return restore_network_policy(db);
}

auto restore_network_settings(sqlito::ExecutorHandle db)
    -> rstd::async::coro<ProcessResult<empty>> {
    return database_run(rstd::move(db), [](sqlito::Connection& connection) {
        return restore_network_settings(connection);
    });
}

auto get_network_settings(sqlito::Connection& db, i32 request_id) -> ProcessResult<QcmMessage> {
    auto settings = rstd_try(network_settings_snapshot(db));
    auto rsp      = Box<p_qcm::p_msg::M_GetNetworkSettingsRsp>::make();
    rsp->f_proxies_json =
        rstd_try(rstd::json::encode(settings.proxies), ProcessError::internal(rstd::move(_e)));
    rsp->f_revision = settings.revision;
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}

auto update_network_settings(sqlito::Connection& db, i32 request_id,
                             const p_qcm::p_msg::M_UpdateNetworkSettingsReq& entry)
    -> ProcessResult<QcmMessage> {
    if (entry.f_proxies_json.len() > usize(512 * 1024))
        return Err(ProcessError::internal("Proxy settings are too large"_str));
    auto proxies =
        rstd_try(rstd::json::decode<Vec<network::ProxyEntry>>(entry.f_proxies_json.as_str()),
                 ProcessError::internal("Invalid proxy list"_str));
    auto compiled = rstd_try(
        network::compile(network::NetworkSettings { entry.f_revision, rstd::move(proxies) }),
        ProcessError::internal(rstd::move(_e)));
    rstd_try(store_network_settings(db, rstd::move(compiled)));
    return Ok(network_api::ok_rsp(request_id));
}
} // namespace qcm::backend
