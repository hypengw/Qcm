export module qcm.backend:api.resource_http;
import :api.http;
import liserveto;
import :resource.file_delivery;
using rstd::sync::Arc;

export namespace qcm::backend
{
auto resource_request_headers(const liserveto::Request& request) -> lihttpto::Headers;
auto resource_response(resource_cache::Response source) -> liserveto::Response;
auto resource_response(resource_cache::ResourceDelivery delivery) -> HttpResponseResult;
auto install_resource_http_routes(liserveto::Router& router, Arc<QcmService> service) -> void;
auto resource_http_dispatch(Arc<QcmService> service, liserveto::Endpoint next)
    -> liserveto::Endpoint;
} // namespace qcm::backend
