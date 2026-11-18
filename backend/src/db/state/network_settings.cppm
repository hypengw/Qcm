export module qcm.backend:db.state.network_settings;
export import :error;
export import :network;
export import sqlito;
using namespace rstd::prelude;

namespace qcm::backend::network_settings_db
{
auto load(sqlito::Transaction& db) -> ProcessResult<network::NetworkSettings>;
} // namespace qcm::backend::network_settings_db

export namespace qcm::backend
{
auto network_settings_snapshot(sqlito::Connection& db) -> ProcessResult<network::NetworkSettings>;
auto restore_network_policy(sqlito::Connection& db) -> ProcessResult<empty>;
auto store_network_settings(sqlito::Connection& db, network::CompiledSettings compiled)
    -> ProcessResult<empty>;
} // namespace qcm::backend
