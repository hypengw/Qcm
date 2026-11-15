module;
#include <rstd/macro.hpp>
module qcm.player.io;
import lihttpto;
import lifetchto;
import rstd.json;
import rstd.serde;
import rstd.log;

using namespace rstd::prelude;
using namespace rstd::literals;
using lihttpto::Url;
using rstd::u16;
using rstd::u64;
using rstd::u8;
using rstd::usize;
using rstd::async::coro;
using rstd::async::Runtime;
using rstd::async::RuntimeBuilder;
using rstd::bytes::Bytes;
using rstd::path::Path;
using rstd::sync::Mutex;
using rstd::time::Duration;
using rstd::time::Instant;
namespace wa   = wavsen::audio;
namespace http = lifetchto::http;

namespace qcm::player_process
{
constexpr usize CONTROL_LIMIT { 65536 };

struct Interrupt {
    void* context {};
    bool (*callback)(void*) noexcept {};
    auto cancelled() const -> bool { return callback && callback(context); }
};

struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", encode_only)]]
EmptyRequest {};
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", encode_only)]]
ResolveRequest {
    String uri;
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", decode_only, ignore_unknown)]] SourceReply {
    String                                                      url;
    [[pmacro::helper("serde::derive", default)]] Option<String> session;
    [[pmacro::helper("serde::derive", default)]] bool           time_seekable {};
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", decode_only, ignore_unknown)]] ResolveReply {
    String url;
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", decode_only, ignore_unknown)]] EmptyReply {};

class ControlHttp {
    Option<Runtime>             runtime_;
    Option<lifetchto::Engine>   engine_;
    Option<http::NativeBackend> backend_;
    Option<http::Client>        client_;

    auto start() -> Result<empty, String> {
        // MediaIo cleanup can move to another caller thread after the last request.
        runtime_ = Some(
            rstd_try(RuntimeBuilder::multi_thread().worker_threads(usize(1)).enable_all().build(),
                     [](auto error) {
                         return rstd::format("resource runtime: {}", error);
                     }));
        engine_ = Some(rstd_try(lifetchto::Engine::make(runtime_->handle()), [](auto error) {
            return rstd::format("resource engine: {}", error);
        }));
        http::TransportOptions transport;
        transport.proxy                      = http::ProxyOptions::disabled();
        transport.max_connections            = usize(2);
        transport.max_connections_per_origin = usize(2);
        transport.max_idle                   = usize(1);
        transport.max_idle_per_origin        = usize(1);
        transport.receive_window             = usize(4096);
        transport.head_limit                 = CONTROL_LIMIT;
        backend_                             = Some(rstd_try(
            http::NativeBackend::make(engine_->resources(), rstd::move(transport)), [](auto error) {
                return rstd::format("resource HTTP backend: {}", error);
            }));
        http::ClientConfig config;
        config.policy.redirect = None();
        config.policy.retry    = None();
        client_ = Some(rstd_try(http::Client::make(engine_->handle(),
                                                   http::BackendBinding::Owned(backend_->backend()),
                                                   rstd::move(config)),
                                [](auto error) {
                                    return rstd::format("resource HTTP client: {}", error);
                                }));
        return Ok(empty {});
    }

    auto shutdown() -> coro<empty> {
        if (client_.is_some()) (void)co_await client_->close();
        if (backend_.is_some()) (void)co_await backend_->close();
        if (engine_.is_some()) (void)co_await engine_->close();
        co_return empty {};
    }

    static auto collect(lifetchto::Transfer<http::ResponseMessage>& transfer)
        -> coro<Result<String, String>> {
        auto response = co_await transfer.output();
        if (response.is_err()) {
            (void)co_await transfer.control().wait();
            co_return Err(rstd::format("resource HTTP response: {}", response.unwrap_err()));
        }
        auto           message = rstd::move(response).unwrap();
        Option<String> error;
        if (message.head.status.value() != u16(200))
            error = Some(rstd::format("resource HTTP status {}", message.head.status.value()));
        Vec<u8> body;
        while (error.is_none()) {
            auto chunk = co_await message.body.next();
            if (chunk.is_err()) {
                error = Some(rstd::format("resource HTTP body: {}", chunk.unwrap_err()));
                break;
            }
            auto next = rstd::move(chunk).unwrap();
            if (next.is_none()) break;
            if (next->len() > CONTROL_LIMIT - body.len()) {
                error = Some("resource control response exceeds 64 KiB"_Str);
                break;
            }
            body.extend_from_slice(next->as_slice());
        }
        if (error.is_some()) {
            message.body.cancel();
            transfer.control().cancel();
            (void)co_await message.body.close();
        }
        auto report = co_await transfer.control().wait();
        if (error.is_some()) co_return Err(rstd::move(*error));
        if (report->logical.result.is_err())
            co_return Err(report->logical.result.as_ref().unwrap_err().message.clone());
        if (report->cleanup.is_err()) co_return Err("resource transfer cleanup failed"_Str);
        auto late_error = transfer.take_error();
        if (late_error.is_some()) co_return Err(rstd::format("resource transfer: {}", *late_error));
        auto text = String::from_utf8(rstd::move(body));
        if (text.is_err()) co_return Err("resource response is not UTF-8"_Str);
        co_return Ok(rstd::move(text).unwrap());
    }

    auto request(const Url& url, String body, Duration timeout, Interrupt interrupt)
        -> coro<Result<String, String>> {
        if (interrupt.cancelled()) co_return Err("resource request cancelled"_Str);
        lihttpto::Headers headers;
        auto              added = headers.add("content-type"_str, "application/json"_str);
        if (added.is_err()) co_return Err("invalid resource request headers"_Str);
        auto                   method = lihttpto::Method::parse("POST"_str).unwrap();
        http::ExecutionRequest input { { url.clone(), rstd::move(method), rstd::move(headers) },
                                       http::UploadPlan::bytes(
                                           Bytes::copy_from_slice(body.as_str().as_bytes())) };
        http::ExecuteOptions   options;
        options.total_timeout         = Some(http::TotalTimeout::after(timeout));
        options.transfer.max_attempts = usize(1);
        auto submitted                = client_->execute(rstd::move(input), rstd::move(options));
        if (submitted.is_err()) {
            auto rejected = rstd::move(submitted).unwrap_err();
            (void)co_await rejected.input.body.close();
            co_return Err(rstd::format("resource HTTP request: {}", rejected.error));
        }
        auto transfer = rstd::move(submitted).unwrap();
        auto reading  = collect(transfer).into_future();
        for (;;) {
            if (interrupt.cancelled()) {
                transfer.control().cancel();
                (void)co_await rstd::move(reading);
                co_return Err("resource request cancelled"_Str);
            }
            auto selected = co_await rstd::async::select(
                rstd::future::by_ref(reading), rstd::async::sleep(Duration::from_millis(u64(25))));
            if (selected.is_left()) co_return rstd::move(selected).unwrap_left();
        }
    }

public:
    ~ControlHttp() { close(); }

    template<class Reply, class Request>
    auto post(const Url& url, const Request& body, Duration timeout, Interrupt interrupt = {})
        -> Result<Reply, String> {
        if (interrupt.cancelled()) return Err("resource request cancelled"_Str);
        auto encoded = rstd_try(rstd::json::encode(body), [](auto error) {
            return rstd::format("resource request JSON: {}", error);
        });
        if (encoded.len() > CONTROL_LIMIT) return Err("resource request exceeds 64 KiB"_Str);
        if (client_.is_none()) {
            auto initialized = start();
            if (initialized.is_err()) {
                auto error = rstd::move(initialized).unwrap_err();
                close();
                return Err(rstd::move(error));
            }
        }
        auto response =
            rstd_try(runtime_->block_on(request(url, rstd::move(encoded), timeout, interrupt)));
        return rstd::json::decode_direct<Reply>(response.as_str()).map_err([](auto error) {
            return rstd::format("resource response JSON: {}", error);
        });
    }

    void close() noexcept {
        if (runtime_.is_none()) return;
        (void)runtime_->block_on(shutdown());
        client_  = None();
        backend_ = None();
        engine_  = None();
        runtime_ = None();
    }
};

auto valid_session(ref<str> token) -> bool {
    if (token.is_empty() || token == "."_str || token == ".."_str) return false;
    for (auto c : token.as_bytes()) {
        if (! rstd::ascii::is_alnum(c) && c != u8('-') && c != u8('_') && c != u8('.') &&
            c != u8('~'))
            return false;
    }
    return true;
}

class ResourceSession {
    Url         source_;
    Option<Url> root_;
    String      session_, prefix_;
    bool        time_seekable_ {};
    Instant     touched_ { Instant::now() };
    ControlHttp http_;

    auto local(const Url& url) const -> bool {
        return source_.same_http_origin(url) && url.userinfo().is_none() &&
               url.fragment().is_none();
    }
    auto control(ref<str> action) const -> Result<Url, String> {
        return source_.resolve(rstd::format("{}/{}", prefix_, action).as_str())
            .map_err([](auto error) {
                return rstd::format("invalid resource control URL: {}", error);
            });
    }
    auto open(Interrupt interrupt) -> Result<empty, String> {
        auto reply = rstd_try(http_.post<SourceReply>(
            source_, EmptyRequest {}, Duration::from_secs(u64(30)), interrupt));
        if (reply.session.is_some() && ! reply.session->is_empty()) {
            if (! valid_session(reply.session->as_str()))
                return Err("invalid resource session token"_Str);
            session_ = rstd::move(*reply.session);
            prefix_  = rstd::format("/playback/{}", session_);
        }
        if (reply.url.is_empty()) {
            close();
            return Err("empty resource media URL"_Str);
        }
        auto root = source_.resolve(reply.url.as_str());
        if (root.is_err() || ! local(*root) ||
            (! session_.is_empty() &&
             ! root->path().starts_with(rstd::format("{}/r/", prefix_).as_str()))) {
            close();
            return Err("resource media URL escaped the local session"_Str);
        }
        root_          = Some(rstd::move(*root));
        time_seekable_ = reply.time_seekable;
        touched_       = Instant::now();
        return Ok(empty {});
    }

public:
    explicit ResourceSession(Url source): source_(rstd::move(source)) {}

    auto resolve(const wa::MediaIoRequest& request) -> Result<wa::MediaInput, String> {
        Interrupt interrupt { request.context, request.cancelled };
        if (interrupt.cancelled()) return Err("resource request cancelled"_Str);
        if (root_.is_none()) rstd_try(open(interrupt));
        if (interrupt.cancelled()) return Err("resource request cancelled"_Str);
        if (request.uri == source_.as_ref() || request.uri == root_->as_ref())
            return Ok(wa::MediaInput { String::make(root_->as_ref()), {}, time_seekable_ });
        if (session_.is_empty()) return Err("resource source has no resolve session"_Str);
        auto target = rstd_try(root_->resolve(request.uri), [](auto error) {
            return rstd::format("invalid resource media URL: {}", error);
        });
        auto prefix = rstd::format("{}/r/", prefix_);
        if (! local(target) || ! target.path().starts_with(prefix.as_str())) {
            auto endpoint = rstd_try(control("resolve"_str));
            auto reply =
                rstd_try(http_.post<ResolveReply>(endpoint,
                                                  ResolveRequest { String::make(target.as_ref()) },
                                                  Duration::from_secs(u64(10)),
                                                  interrupt));
            if (reply.url.is_empty()) return Err("empty resolved resource URL"_Str);
            target = rstd_try(source_.resolve(reply.url.as_str()), [](auto error) {
                return rstd::format("invalid resolved resource URL: {}", error);
            });
            if (! local(target) || ! target.path().starts_with(prefix.as_str()))
                return Err("resolved media URL escaped the local session"_Str);
        }
        if (interrupt.cancelled()) return Err("resource request cancelled"_Str);
        return Ok(wa::MediaInput { String::make(target.as_ref()), {}, time_seekable_ });
    }

    void keep_alive() {
        if (session_.is_empty() || touched_.elapsed() < Duration::from_secs(u64(30))) return;
        touched_      = Instant::now();
        auto endpoint = control("touch"_str);
        if (endpoint.is_ok())
            (void)http_.post<EmptyReply>(*endpoint, EmptyRequest {}, Duration::from_secs(u64(2)));
    }

    void close() noexcept {
        Option<Url> endpoint;
        if (! session_.is_empty()) {
            auto parsed = control("close"_str);
            if (parsed.is_ok()) endpoint = Some(rstd::move(*parsed));
        }
        session_.clear();
        prefix_.clear();
        root_          = None();
        time_seekable_ = false;
        if (endpoint.is_some())
            (void)http_.post<EmptyReply>(*endpoint, EmptyRequest {}, Duration::from_secs(u64(2)));
        http_.close();
    }
};

class ResourceIo final : public DefaultInClass<ResourceIo, wa::MediaIo> {
    Mutex<ResourceSession> state_;

public:
    explicit ResourceIo(Url source): state_(rstd::move(source)) {}
    ~ResourceIo() { close(); }
    auto resolve(const wa::MediaIoRequest& request) -> Result<wa::MediaInput, wa::MediaError> {
        auto state  = state_.lock().unwrap();
        auto result = state->resolve(request);
        if (result.is_ok()) return Ok(rstd::move(*result));
        const bool cancelled = request.cancelled && request.cancelled(request.context);
        if (! cancelled) rstd::log::warn("qcm-player resource IO: {}", result.unwrap_err());
        return Err(wa::MediaError { cancelled ? wa::MediaErrorKind::Cancelled
                                              : wa::MediaErrorKind::Open });
    }
    void keep_alive() { state_.lock().unwrap()->keep_alive(); }
    void close() noexcept { state_.lock().unwrap()->close(); }
};

auto parse_source(ref<str> input) -> Result<MediaSource, String> {
    if (input.contains("\0"_str)) return Err("source contains NUL"_Str);
    if (input.is_empty()) return Ok(MediaSource { {}, {}, false });
    if (ref<Path>(input).has_root()) return Ok(MediaSource { String::make(input), {}, true });
    auto parsed = Url::parse(input);
    if (parsed.is_err()) {
        if (parsed.as_ref().unwrap_err().kind().is_MissingScheme())
            return Ok(MediaSource { String::make(input), {}, true });
        return Err(rstd::format("invalid source URL: {}", parsed.unwrap_err()));
    }
    auto url = rstd::move(parsed).unwrap();
    if (url.scheme() == Some("file"_str)) {
        auto path = rstd_try(url.to_file_path(), [](auto error) {
            return rstd::format("invalid file source: {}", error);
        });
        auto text = path.as_path().to_str();
        if (text.is_none()) return Err("file source path is not UTF-8"_Str);
        return Ok(MediaSource { String::make(*text), {}, true });
    }
    if (url.scheme() != Some("http"_str) && url.scheme() != Some("https"_str))
        return Err("unsupported media source scheme"_Str);
    auto uri = String::make(url.as_ref());
    if (url.scheme() == Some("http"_str) &&
        (url.host_name() == Some("127.0.0.1"_str) || url.host_name() == Some("::1"_str)) &&
        url.path().starts_with("/playback/source/"_str)) {
        if (url.userinfo().is_some() || url.fragment().is_some())
            return Err("invalid backend playback source"_Str);
        auto io = Box<ResourceIo>::make(rstd::move(url));
        return Ok(MediaSource {
            rstd::move(uri), Some(rstd::move(io).into_dyn<dyn<wa::MediaIo>>()), false });
    }
    return Ok(MediaSource { rstd::move(uri), {}, false });
}
} // namespace qcm::player_process
