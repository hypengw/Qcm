export module qcm.backend:api.http;
import :error;
import :event;
import liserveto;
import liproto;
import rstd;
using rstd::async::coro;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
class QcmService;
class SessionRegistry;
using HttpResponseResult = ProcessResult<liserveto::Response>;

auto qcm_http_response(HttpResponseResult result) -> liserveto::ResponseResult;

template<typename Handler>
auto invoke_qcm_http_handler(Arc<Handler> handler, liserveto::Request request)
    -> coro<liserveto::ResponseResult> {
    co_return qcm_http_response(co_await (*handler)(rstd::move(request)));
}

template<typename Handler>
    requires rstd::mtp::same_as<rstd::mtp::invoke_result_t<const Handler&, liserveto::Request>,
                                coro<HttpResponseResult>>
auto qcm_http_endpoint(Handler handler) -> liserveto::Endpoint {
    auto owner = Arc<Handler>::make(rstd::move(handler));
    return liserveto::Endpoint::make([owner = rstd::move(owner)](liserveto::Request request) {
        return invoke_qcm_http_handler(owner.clone(), rstd::move(request));
    });
}

auto qcm_encode(const QcmMessage& message) -> Result<Vec<u8>, liproto::Error>;
auto qcm_http_dispatch(Arc<QcmService> service, Arc<SessionRegistry> sessions,
                       liserveto::Endpoint routes) -> liserveto::Endpoint;
auto parameter_text(const liserveto::Request& request, ref<str> name) -> String;
auto parse_i64(ref<str> text) -> Option<i64>;
auto text_byte_index(ref<str> text, char needle, rstd::size_t from) -> Option<rstd::size_t>;
auto text_byte_slice(ref<str> text, rstd::size_t begin, rstd::size_t end) -> String;
} // namespace qcm::backend
