module;
#include <rstd/macro.hpp>
module qcm.backend;
import :api.http;
import :error_response;
import :service;
import :session;
import liserveto;
import liproto;
import rstd.log;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::coro;
using rstd::bytes::Bytes;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
auto qcm_http_response(HttpResponseResult result) -> liserveto::ResponseResult {
    if (result.is_err()) return Ok(http_error_response(result.as_ref().unwrap_err()));
    return Ok(rstd::move(*result));
}

auto qcm_encode(const QcmMessage& message) -> Result<Vec<u8>, liproto::Error> {
    return liproto::encode_to_vec(message);
}

static auto
complete_http_response(Rc<Mutex<rstd::async::oneshot::Sender<ProcessResult<QcmMessage>>>> sender,
                       ProcessResult<QcmMessage> response) -> coro<ProcessResult<empty>> {
    auto guard = sender->lock().unwrap();
    (void)guard->send(rstd::move(response));
    co_return Ok(empty {});
}

static auto process_http_message(Arc<QcmService> service, QcmMessage request,
                                 Arc<ProtocolSession> protocol) -> coro<ProcessResult<QcmMessage>> {
    auto channel = rstd::async::oneshot::channel<ProcessResult<QcmMessage>>();
    auto sender  = make_rc<Mutex<rstd::async::oneshot::Sender<ProcessResult<QcmMessage>>>>(
        rstd::move(channel.get<0>()));
    auto sink =
        ResponseSink::make([sender = rstd::move(sender)](i32, ProcessResult<QcmMessage> response) {
            return complete_http_response(sender.clone(), rstd::move(response));
        });
    auto processed = rstd_co_try(co_await service->process_request(
        service.clone(), rstd::move(request), *protocol, rstd::move(sink)));
    if (processed.is_some()) co_return Ok(rstd::move(*processed));
    auto response = co_await rstd::move(channel.get<1>());
    if (response.is_err())
        co_return Err(ProcessError::internal("request cancelled before response"_str));
    co_return rstd::move(*response);
}

static auto http_process_response(i32 id, ProcessResult<QcmMessage> result) -> HttpResponseResult {
    if (result.is_err())
        rstd::log::error("request {} failed: {}", id, result.as_ref().unwrap_err());
    auto response =
        result.is_ok() ? rstd::move(*result) : error_response(id, rstd::move(result).unwrap_err());
    response.f_id = id;
    auto encoded  = rstd_try(qcm_encode(response), [](auto error) {
        return ProcessError::encode(rstd::move(error));
    });
    return Ok(liserveto::Response::bytes(Bytes::copy_from_slice(encoded.as_slice())));
}

static auto qcm_http_post(Arc<QcmService> service, Arc<SessionSlot> session,
                          liserveto::Request request) -> coro<HttpResponseResult> {
    auto collected = rstd_co_try(co_await request.collect(u64(65536)), [](auto error) {
        return ProcessError::internal(rstd::move(error));
    });
    auto decoded   = liproto::decode<QcmMessage>(collected.data.as_slice());
    if (decoded.is_err())
        co_return http_process_response(
            i32(), Err(ProcessError::decode(rstd::move(decoded).unwrap_err())));
    const auto id       = decoded->f_id;
    auto       response = co_await rstd::async::select(
        rstd::async::into_future(
            process_http_message(service.clone(), rstd::move(*decoded), session->protocol())),
        rstd::async::select(request.shutdown.notified(), request.lifetime.finished()));
    if (response.is_right()) co_return Err(ProcessError::internal("HTTP request cancelled"_str));
    co_return http_process_response(id, rstd::move(response).unwrap_left());
}

static auto qcm_http_post_request(Arc<QcmService> service, Arc<SessionRegistry> sessions,
                                  liserveto::Request request) -> coro<HttpResponseResult> {
    auto session = sessions->active_session();
    if (session.is_none()) {
        liserveto::Response unavailable;
        unavailable.head.status = lihttpto::StatusCode::make(u16(503)).unwrap();
        co_return Ok(rstd::move(unavailable));
    }
    co_return co_await qcm_http_post(service.clone(), rstd::move(*session), rstd::move(request));
}

static auto dispatch_qcm_http(liserveto::Endpoint post, liserveto::Endpoint routes,
                              liserveto::Request request) -> coro<liserveto::ResponseResult> {
    if (request.head().line.method.as_str() == "POST"_str &&
        ! request.head().line.resource.escaped_path.as_str().starts_with("/playback/"_str))
        co_return co_await post.call(rstd::move(request));
    co_return co_await routes.call(rstd::move(request));
}

auto qcm_http_dispatch(Arc<QcmService> service, Arc<SessionRegistry> sessions,
                       liserveto::Endpoint routes) -> liserveto::Endpoint {
    auto post = qcm_http_endpoint([service  = rstd::move(service),
                                   sessions = rstd::move(sessions)](liserveto::Request request) {
        return qcm_http_post_request(service.clone(), sessions.clone(), rstd::move(request));
    });
    return liserveto::Endpoint::make(
        [post = rstd::move(post), routes = rstd::move(routes)](liserveto::Request request) {
            return dispatch_qcm_http(post.clone(), routes.clone(), rstd::move(request));
        });
}

auto parameter_text(const liserveto::Request& request, ref<str> name) -> String {
    auto value = request.parameter(name);
    if (value.is_none()) return String::make();
    auto bytes = Vec<u8>::make();
    for (const auto& byte : *value) bytes.push(u8(byte));
    return String::from_utf8_unchecked(rstd::move(bytes));
}

auto parse_i64(ref<str> text) -> Option<i64> {
    if (text.is_empty()) return None();
    i64 value {};
    for (auto byte : text.as_bytes()) {
        auto digit = byte.to_primitive();
        if (digit < '0' || digit > '9') return None();
        auto number = static_cast<long long>(digit - '0');
        if (value.to_primitive() > (i64::MAX.to_primitive() - number) / 10) return None();
        value = i64(value.to_primitive() * 10 + number);
    }
    return Some(value);
}

auto text_byte_index(ref<str> text, char needle, rstd::size_t from) -> Option<rstd::size_t> {
    auto bytes = text.as_bytes();
    for (rstd::size_t index = from; index < bytes.len().to_primitive(); ++index)
        if (bytes[usize(index)].to_primitive() == static_cast<unsigned char>(needle))
            return Some(index);
    return None();
}

auto text_byte_slice(ref<str> text, rstd::size_t begin, rstd::size_t end) -> String {
    auto bytes = text.as_bytes();
    auto out   = Vec<u8>::make();
    for (rstd::size_t index = begin; index < end && index < bytes.len().to_primitive(); ++index)
        out.push(u8(bytes[usize(index)]));
    return String::from_utf8_unchecked(rstd::move(out));
}
} // namespace qcm::backend
