module;
#include <rstd/macro.hpp>
export module qcm.backend:server;
export import :api.http;
import :api.playback_http;
import :api.resource_http;
export import :service;
export import rstd;
export import liserveto;
export import liwebsocketo;
import liproto;
import lihttpto;
import rstd.log;
import :error_response;
import :session;
using rstd::async::AbortOnDropHandle;
using rstd::net::TcpListener;
using rstd::sync::Arc;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::async::coro;
using rstd::bytes::Bytes;

export namespace qcm::backend
{
struct EncodedProcessResponse {
    i32     type;
    bool    handshake;
    Vec<u8> bytes;
};
auto encode_process_response(i32 id, Result<QcmMessage, ProcessError> result) {
    if (result.is_err())
        rstd::log::error("request {} failed: {}", id, result.as_ref().unwrap_err());
    auto response =
        result.is_ok() ? rstd::move(*result) : error_response(id, rstd::move(result).unwrap_err());
    auto encoded = qcm_encode(response);
    if (encoded.is_err()) {
        response = error_response(id, ProcessError::encode(rstd::move(encoded).unwrap_err()));
        encoded  = qcm_encode(response);
    }
    return rstd::move(encoded).map([&](Vec<u8> bytes) {
        return EncodedProcessResponse { response.f_type,
                                        ProtocolSession::is_handshake_response(response),
                                        rstd::move(bytes) };
    });
}
auto websocket_response_sink(Arc<SessionSlot> slot) -> ResponseSink {
    return ResponseSink::make([slot = rstd::move(slot)](i32 id, ProcessResult<QcmMessage> response)
                                  -> coro<ProcessResult<empty>> {
        auto encoded =
            rstd_co_try(encode_process_response(id, rstd::move(response)), [](auto error) {
                return ProcessError::encode(rstd::move(error));
            });
        rstd_co_try(co_await slot->push(Bytes::copy_from_slice(encoded.bytes.as_slice())),
                    [](auto error) {
                        return ProcessError::internal(rstd::move(error));
                    });
        co_return Ok(empty {});
    });
}
auto websocket_event_sink(Arc<SessionRegistry> sessions) -> EventSink {
    return EventSink::make(
        [sessions = rstd::move(sessions)](QcmMessage message) -> coro<ProcessResult<empty>> {
            auto encoded = rstd_co_try(qcm_encode(message), [](auto error) {
                return ProcessError::encode(rstd::move(error));
            });
            rstd_co_try(co_await sessions->broadcast(Bytes::copy_from_slice(encoded.as_slice())),
                        [](auto error) {
                            return ProcessError::internal(rstd::move(error));
                        });
            co_return Ok(empty {});
        });
}
auto qcm_websocket_message(liwebsocketo::Message message, Arc<QcmService> service,
                           Arc<SessionSlot> session_slot, Arc<SessionRegistry> sessions,
                           liserveto::Shutdown stopped) -> coro<empty> {
    if (message.kind == liwebsocketo::Kind::Text) {
        auto text = rstd::str_::from_utf8(message.data.as_slice());
        if (text.is_err()) {
            (void)stopped.request_stop();
            co_return empty {};
        }
        auto echo = String::make("Echo: "_str);
        echo.push_str(*text);
        auto sent = co_await session_slot->send(SessionFrame {
            liwebsocketo::Kind::Text, Bytes::copy_from_slice(echo.as_str().as_bytes()) });
        if (sent.is_err()) (void)stopped.request_stop();
        co_return empty {};
    }
    if (message.kind != liwebsocketo::Kind::Binary) co_return empty {};
    auto decoded = liproto::decode<QcmMessage>(message.data.as_slice());
    if (decoded.is_err()) {
        auto encoded = encode_process_response(
            i32(), Err(ProcessError::decode(rstd::move(decoded).unwrap_err())));
        if (encoded.is_ok())
            (void)co_await session_slot->push(Bytes::copy_from_slice(encoded->bytes.as_slice()));
        co_return empty {};
    }
    const auto                             request_id = decoded->f_id;
    auto                                   protocol   = session_slot->protocol();
    Option<rstd::async::MutexGuard<empty>> handshake;
    if (ProtocolSession::is_handshake(*decoded)) handshake = Some(co_await protocol->handshake());
    auto processed =
        co_await service->process_request(service.clone(),
                                          rstd::move(*decoded),
                                          *protocol,
                                          websocket_response_sink(session_slot.clone()));
    if (processed.is_ok() && processed->is_none()) co_return empty {};
    ProcessResult<QcmMessage> finalized =
        processed.is_err() ? ProcessResult<QcmMessage>(Err(rstd::move(processed).unwrap_err()))
                           : ProcessResult<QcmMessage>(Ok(rstd::move(**processed)));
    auto encoded = encode_process_response(request_id, rstd::move(finalized));
    if (encoded.is_err()) {
        rstd::log::error("encode qcm response failed: {}", encoded.as_ref().unwrap_err());
        (void)stopped.request_stop();
        co_return empty {};
    }
    rstd::log::debug(
        "response type {} bytes {}", encoded->type, encoded->bytes.len().to_primitive());
    auto sent = co_await session_slot->push(Bytes::copy_from_slice(encoded->bytes.as_slice()));
    if (sent.is_err()) {
        (void)stopped.request_stop();
        co_return empty {};
    }
    if (encoded->handshake) {
        auto completed = protocol->complete_handshake();
        if (completed.is_err()) {
            (void)stopped.request_stop();
            co_return empty {};
        }
        if (! *completed) co_return empty {};
        auto messages = co_await service->initial_messages();
        if (messages.is_err()) {
            protocol->close();
            auto encoded =
                encode_process_response(request_id, Err(rstd::move(messages).unwrap_err()));
            if (encoded.is_ok())
                (void)co_await session_slot->push(
                    Bytes::copy_from_slice(encoded->bytes.as_slice()));
            co_return empty {};
        }
        for (auto& initial : *messages) {
            auto initial_encoded = qcm_encode(initial);
            if (initial_encoded.is_err()) {
                protocol->close();
                (void)stopped.request_stop();
                co_return empty {};
            }
            auto pushed =
                co_await session_slot->push(Bytes::copy_from_slice(initial_encoded->as_slice()));
            if (pushed.is_err()) {
                (void)stopped.request_stop();
                co_return empty {};
            }
        }
        if ((co_await sessions->flush_ui_activation()).is_err()) (void)stopped.request_stop();
    }

    co_return empty {};
}

class WebSocketRequests {
    array<Option<AbortOnDropHandle<empty>>, 256> jobs_;
    rstd::async::Notify completed_ { rstd::async::Notify::make().unwrap() };

    static auto run(coro<empty> request, rstd::async::Notify completed) -> coro<empty> {
        struct Completion {
            rstd::async::Notify completed;
            ~Completion() { (void)completed.notifier().notify(); }
        } completion { rstd::move(completed) };
        co_await rstd::move(request);
        co_return empty {};
    }

public:
    auto start(coro<empty> request, const liserveto::Shutdown& stopped) -> coro<bool> {
        for (;;) {
            if (stopped.requested()) co_return false;
            for (auto& job : jobs_) {
                if (job.is_some() && ! job->is_finished()) continue;
                job = Some(AbortOnDropHandle<empty> {
                    rstd::async::spawn_local(run(rstd::move(request), completed_.clone())) });
                co_return true;
            }
            auto wake = co_await rstd::async::select(completed_.notified(), stopped.notified());
            if (wake.is_right()) co_return false;
        }
    }
    void cancel() {
        for (auto& job : jobs_)
            if (job.is_some()) job->abort();
    }
    auto finish() -> coro<empty> {
        cancel();
        for (auto& job : jobs_) {
            if (job.is_none()) continue;
            (void)co_await rstd::move(*job);
            job = None();
        }
        co_return empty {};
    }
};

auto qcm_websocket_writer(liserveto::WebSocket& socket, Arc<SessionSlot> slot,
                          liserveto::Shutdown stopped)
    -> coro<Result<empty, liserveto::BodyError>> {
    for (;;) {
        auto next = co_await rstd::async::select(slot->recv().into_future(), stopped.notified());
        if (next.is_right()) co_return Ok(empty {});
        auto frame = rstd::move(next).unwrap_left();
        if (frame.is_none()) co_return Ok(empty {});
        auto sent =
            co_await rstd::async::select(socket.send(rstd::move(*frame)), stopped.notified());
        if (sent.is_right()) co_return Ok(empty {});
        auto result = rstd::move(sent).unwrap_left();
        if (result.is_err() || result->is_err()) {
            rstd::log::warn("websocket writer stopped");
            co_return Err(liserveto::BodyError::Io);
        }
    }
}
auto qcm_websocket_reader(liserveto::WebSocket::MessageStream messages, Arc<QcmService> service,
                          Arc<SessionSlot> slot, Arc<SessionRegistry> sessions,
                          Arc<WebSocketRequests> requests, liserveto::Shutdown stopped,
                          liwebsocketo::Message first)
    -> coro<Result<empty, liserveto::BodyError>> {
    if (! co_await requests->start(qcm_websocket_message(rstd::move(first),
                                                         service.clone(),
                                                         slot.clone(),
                                                         sessions.clone(),
                                                         stopped.clone()),
                                   stopped))
        co_return Ok(empty {});
    for (;;) {
        auto next = co_await rstd::async::select(
            rstd::async::into_future(rstd::stream::next(messages)), stopped.notified());
        if (next.is_right()) co_return Ok(empty {});
        auto message = rstd::move(next).unwrap_left();
        if (message.is_none()) co_return Ok(empty {});
        auto frame = rstd_co_try(rstd::move(*message), liserveto::BodyError::Protocol);
        if (frame.kind == liwebsocketo::Kind::Close) co_return Ok(empty {});
        if (! co_await requests->start(qcm_websocket_message(rstd::move(frame),
                                                             service.clone(),
                                                             slot.clone(),
                                                             sessions.clone(),
                                                             stopped.clone()),
                                       stopped))
            co_return Ok(empty {});
    }
}

auto reject_duplicate_ui(liserveto::WebSocket& socket, Arc<SessionRegistry> sessions,
                         liwebsocketo::Message frame) -> coro<Result<empty, liserveto::BodyError>> {
    if (frame.kind != liwebsocketo::Kind::Binary) co_return Ok(empty {});
    auto decoded = liproto::decode<QcmMessage>(frame.data.as_slice());
    if (decoded.is_err() || decoded->f_protocol_version != qcm_protocol_version ||
        ! ProtocolSession::is_handshake(*decoded) ||
        ! decoded->o_payload->as_test_req()->f_ui_client)
        co_return Ok(empty {});
    auto activated = co_await sessions->activate_ui(
        decoded->o_payload->as_test_req()->f_activation_token.clone());
    if (activated.is_err()) co_return Err(liserveto::BodyError::Io);
    auto response           = Box<p_qcm::p_msg::M_TestRsp>::make();
    response->f_ui_accepted = Some(false);
    auto encoded = rstd_co_try(qcm_encode(qcm_message(decoded->f_id, rstd::move(response))),
                               liserveto::BodyError::Protocol);
    auto sent    = co_await socket.send(
        { liwebsocketo::Kind::Binary, Bytes::copy_from_slice(encoded.as_slice()) });
    if (sent.is_err() || sent->is_err()) co_return Err(liserveto::BodyError::Io);
    (void)co_await socket.close();
    co_return Ok(empty {});
}

auto qcm_websocket(liserveto::WebSocket& socket, Arc<QcmService> service,
                   Arc<SessionRegistry> sessions, liserveto::Shutdown shutdown,
                   liserveto::RequestLifetime lifetime)
    -> coro<Result<empty, liserveto::BodyError>> {
    auto session_slot = rstd_co_try(SessionSlot::make(), liserveto::BodyError::Io);
    auto stopped      = liserveto::Shutdown::make().unwrap();
    auto requests     = Arc<WebSocketRequests>::make();
    struct Registered {
        Arc<SessionRegistry>   sessions;
        Arc<SessionSlot>       slot;
        Arc<WebSocketRequests> requests;
        liserveto::WebSocket&  socket;
        liserveto::Shutdown    stopped;
        Option<u64>            id;
        ~Registered() {
            if (id.is_some()) sessions->remove(*id);
            slot->close();
            requests->cancel();
            socket.cancel();
            (void)stopped.request_stop();
        }
    } registered { sessions.clone(), session_slot.clone(), requests.clone(),
                   socket,           stopped.clone(),      sessions->add(session_slot.clone()) };
    auto messages = rstd_co_try(socket.messages(), liserveto::BodyError::Io);
    auto received = co_await rstd::async::timeout(
        rstd::async::into_future(rstd::stream::next(messages)), Duration::from_secs(u64(5)));
    if (received.is_err() || received->is_none()) co_return Ok(empty {});
    auto first = rstd_co_try(rstd::move(**received), liserveto::BodyError::Protocol);
    if (first.kind == liwebsocketo::Kind::Close) co_return Ok(empty {});
    if (registered.id.is_none()) registered.id = sessions->add(session_slot.clone());
    if (registered.id.is_none())
        co_return co_await reject_duplicate_ui(socket, sessions.clone(), rstd::move(first));
    // Socket-borrowing futures stay scoped to the upgrade callback.
    auto reading = Some(rstd::async::into_future(qcm_websocket_reader(rstd::move(messages),
                                                                      service.clone(),
                                                                      session_slot.clone(),
                                                                      sessions.clone(),
                                                                      requests.clone(),
                                                                      stopped.clone(),
                                                                      rstd::move(first))));
    auto writing = Some(rstd::async::into_future(
        qcm_websocket_writer(socket, session_slot.clone(), stopped.clone())));
    auto events  = Some(rstd::async::into_future(sessions->dispatch(*registered.id)));
    auto result  = co_await rstd::async::select(
        rstd::async::select(rstd::future::by_ref(*reading), rstd::future::by_ref(*writing)),
        rstd::async::select(
            rstd::future::by_ref(*events),
            rstd::async::select(stopped.notified(),
                                rstd::async::select(shutdown.notified(), lifetime.finished()))));
    Result<empty, liserveto::BodyError> outcome = Ok(empty {});
    if (result.is_left()) {
        auto io = rstd::move(result).unwrap_left();
        outcome = io.is_left() ? rstd::move(io).unwrap_left() : rstd::move(io).unwrap_right();
    } else {
        auto other = rstd::move(result).unwrap_right();
        if (other.is_left() && rstd::move(other).unwrap_left().is_err())
            outcome = Err(liserveto::BodyError::Io);
    }
    sessions->remove(*registered.id);
    (void)stopped.request_stop();
    socket.cancel();
    reading = None();
    writing = None();
    events  = None();
    co_await requests->finish();
    co_return rstd::move(outcome);
}

auto websocket_endpoint(Arc<QcmService> service, Arc<SessionRegistry> sessions)
    -> liserveto::Endpoint {
    return liserveto::Endpoint::make([service  = rstd::move(service),
                                      sessions = rstd::move(sessions)](liserveto::Request request)
                                         -> coro<liserveto::ResponseResult> {
        auto upgrade = liserveto::WebSocketUpgrade::from_request(request);
        if (upgrade.is_err())
            co_return rstd::Err(liserveto::Error { liserveto::ErrorKind::InvalidRequest, None() });
        liwebsocketo::WebSocketOptions options;
        options.limits.frame_bytes   = usize(64 * 1024 * 1024);
        options.limits.message_bytes = usize(64 * 1024 * 1024);
        options.receive_bytes        = usize(64 * 1024 * 1024);
        auto response                = rstd::move(upgrade).unwrap().accept(
            [service = service.clone(), sessions = sessions.clone()](
                liserveto::WebSocket&      socket,
                liserveto::Shutdown        shutdown,
                liserveto::RequestLifetime lifetime) -> coro<Result<empty, liserveto::BodyError>> {
                co_return co_await qcm_websocket(socket,
                                                 service.clone(),
                                                 sessions.clone(),
                                                 rstd::move(shutdown),
                                                 rstd::move(lifetime));
            },
            options);
        if (response.is_err())
            co_return rstd::Err(liserveto::Error { liserveto::ErrorKind::InvalidRequest, None() });
        co_return rstd::Ok(rstd::move(response).unwrap());
    });
}
auto qcm_endpoint(Arc<QcmService> service, Arc<SessionRegistry> sessions) -> liserveto::Endpoint {
    liserveto::Router router;
    router.get("/ws"_str, websocket_endpoint(service.clone(), sessions.clone())).unwrap();
    router.get("/"_str, websocket_endpoint(service.clone(), sessions.clone())).unwrap();
    install_resource_http_routes(router, service.clone());
    install_playback_http_routes(router, service.clone());
    auto routes = rstd::move(router).into_endpoint();
    return resource_http_dispatch(
        service.clone(), qcm_http_dispatch(service.clone(), sessions.clone(), rstd::move(routes)));
}
auto watch_sync_shutdown(Arc<QcmService> service, liserveto::Shutdown shutdown) -> coro<empty> {
    (void)co_await shutdown.notified();
    service->cancel_sync_tasks();
    service->begin_provider_shutdown();
    co_return empty {};
}
auto collect_playback_sessions(Arc<QcmService> service) -> coro<empty> {
    for (;;) {
        co_await rstd::async::sleep(Duration::from_secs(u64(15)));
        co_await service->playback().collect();
        co_await service->collect_radio_sessions();
    }
}
auto serve_qcm(TcpListener listener, Arc<QcmService> service, liserveto::Shutdown shutdown,
               liserveto::ServerLimits limits = {}, Option<Arc<SessionRegistry>> registry = {})
    -> coro<Result<liserveto::ServerReport, rstd::io::error::Error>> {
    struct CancelOnDrop {
        QcmService& service;
        ~CancelOnDrop() {
            service.cancel_sync_tasks();
            service.begin_provider_shutdown();
            service.playback().cancel();
            service.set_event_sink(None());
        }
    } cancel_on_drop { *service };
    auto resources = co_await service->start_resource_cache();
    if (resources.is_err()) {
        co_await service->stop_player();
        co_await service->stop_providers();
        co_return Err(rstd::io::Error::with_source(
            rstd::io::ErrorKind { rstd::io::ErrorKind::Other },
            Box<dyn<rstd::error::Error>>::make(rstd::move(resources).unwrap_err())));
    }
    service->playback().start();
    auto watcher = AbortOnDropHandle { rstd::async::spawn_local(
        watch_sync_shutdown(service.clone(), shutdown.clone())) };
    auto playback_collector =
        AbortOnDropHandle { rstd::async::spawn_local(collect_playback_sessions(service.clone())) };
    auto sessions = registry.is_some() ? rstd::move(*registry) : Arc<SessionRegistry>::make();
    service->set_event_sink(Some(websocket_event_sink(sessions.clone())));
    auto endpoint = Arc<liserveto::Endpoint>::make(qcm_endpoint(service.clone(), sessions.clone()));
    auto result   = co_await liserveto::serve(
        rstd::move(listener), rstd::move(endpoint), rstd::move(shutdown), limits);
    watcher.abort();
    (void)co_await rstd::move(watcher);
    playback_collector.abort();
    (void)co_await rstd::move(playback_collector);
    co_await service->stop_player();
    (void)co_await service->stop_provider_changes();
    (void)co_await service->stop_sync_tasks();
    (void)co_await service->playback().stop();
    (void)co_await service->stop_radio_service();
    (void)co_await service->stop_resource_cache();
    (void)co_await service->stop_providers();
    service->set_event_sink(None());
    co_return result;
}
} // namespace qcm::backend
