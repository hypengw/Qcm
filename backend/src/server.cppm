export module qcm.backend:server;
import :service;
import :session;
import liserveto;
import rstd;
using rstd::async::coro;
using rstd::net::TcpListener;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
auto qcm_endpoint(Arc<QcmService> service, Arc<SessionRegistry> sessions) -> liserveto::Endpoint;
auto serve_qcm(TcpListener listener, Arc<QcmService> service, liserveto::Shutdown shutdown,
               liserveto::ServerLimits limits = {}, Option<Arc<SessionRegistry>> registry = {})
    -> coro<Result<liserveto::ServerReport, rstd::io::error::Error>>;
} // namespace qcm::backend
