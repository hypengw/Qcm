module;
#include <rstd/macro.hpp>
module qcm.backend;
import :network.client;
import :network.cookie_codec;
import liuuid;
using rstd::tuple;
using rstd::async::coro;
using rstd::async::RuntimeHandle;
using rstd::bytes::Bytes;
using rstd::collections::BTreeMap;
using rstd::sync::Arc;
using rstd::sync::Mutex;
using rstd::time::Duration;
using rstd::time::SystemTime;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::network
{
namespace
{
Mutex<BTreeMap<i64, ClientPolicy>> network_routes;
auto                               proxy_id(ref<str> input) -> Result<String, lifetchto::Error> {
    if (input.is_empty()) {
        auto value = rstd_try(liuuid::Uuid::new_v4(), lifetchto::Error::Connect(rstd::move(_e)));
        return Ok(value.hyphenated());
    }
    auto value = liuuid::Uuid::parse_str(input);
    if (value.is_err()) return Err(lifetchto::Error::InvalidOptions("Invalid proxy ID"_Str));
    return Ok(value->hyphenated());
}
} // namespace

auto compile(NetworkSettings settings) -> Result<CompiledSettings, lifetchto::Error> {
    if (settings.proxies.len() > usize(64))
        return Err(lifetchto::Error::InvalidOptions("Too many proxies"_Str));
    BTreeMap<i64, ClientPolicy> routes;
    Vec<String>                 ids;
    for (auto& entry : settings.proxies) {
        entry.id = rstd_try(proxy_id(entry.id.as_str()));
        for (const auto& id : ids)
            if (id == entry.id)
                return Err(lifetchto::Error::InvalidOptions("Duplicate proxy ID"_Str));
        ids.push(entry.id.clone());
        entry.name = String::make(entry.name.as_str().trim());
        if (entry.name.is_empty() || entry.name.len() > usize(128))
            return Err(lifetchto::Error::InvalidOptions("Proxy name must contain 1-128 bytes"_Str));
        if (entry.url.len() > usize(4096))
            return Err(lifetchto::Error::InvalidOptions("Proxy address is too long"_Str));
        auto url = lihttpto::Url::parse(entry.url.as_str().trim());
        if (url.is_err()) return Err(lifetchto::Error::InvalidOptions("Invalid proxy address"_Str));
        auto scheme = url->scheme();
        auto port   = url->effective_port();
        if (scheme.is_none() ||
            ! (*scheme == "http"_str || *scheme == "https"_str || *scheme == "socks5"_str ||
               *scheme == "socks5h"_str) ||
            url->host_name().is_none() || port.is_none() || *port == u16() ||
            (! url->path().is_empty() && url->path() != "/"_str) || url->query().is_some() ||
            url->fragment().is_some())
            return Err(lifetchto::Error::InvalidOptions(
                "Use an http, https, socks5 or socks5h proxy address with a valid host and port"_Str));
        entry.url  = String::make(url->as_ref());
        auto proxy = rstd_try(lifetchto::http::ProxyOptions::explicit_proxy(rstd::move(*url)));
        entry.display_url = String::make(proxy.explicit_endpoint()->url().as_ref());
        if (entry.provider_ids.len() > usize(256))
            return Err(lifetchto::Error::InvalidOptions("Too many providers for a proxy"_Str));
        ClientPolicy route { entry.url.clone(), rstd::move(proxy) };
        for (auto id : entry.provider_ids) {
            if (id <= i64() || routes.contains_key(id))
                return Err(
                    lifetchto::Error::InvalidOptions("A provider may use only one proxy"_Str));
            routes.insert(id, route.clone());
        }
    }
    return Ok(CompiledSettings { rstd::move(settings), rstd::move(routes) });
}
auto publish(CompiledSettings compiled) -> void {
    auto routes = network_routes.lock().unwrap();
    *routes     = rstd::move(compiled.routes_);
}
auto policy(Option<i64> provider) -> ClientPolicy {
    if (provider.is_none()) return ClientPolicy {};
    auto routes = network_routes.lock().unwrap();
    auto route  = routes->get(*provider);
    return route.is_some() ? (*route)->clone() : ClientPolicy {};
}
} // namespace qcm::backend::network

namespace qcm::backend
{
auto parse_proxy(ref<str> url) -> Result<NetworkProxy, lifetchto::Error> {
    auto parsed = lihttpto::Url::parse(url);
    if (parsed.is_err())
        return Err(lifetchto::Error::InvalidOptions(String::make("invalid proxy url"_str)));
    return lifetchto::http::ProxyEndpoint::make(rstd::move(parsed).unwrap());
}

auto HttpResponseStream::next() -> coro<Result<Option<Bytes>, lifetchto::Error>> {
    co_return co_await message_.body.next();
}

auto HttpResponseStream::cancel() -> void { message_.body.cancel(); }

auto HttpResponseStream::read(usize limit) -> coro<Result<Vec<u8>, lifetchto::Error>> {
    Vec<u8> body;
    for (;;) {
        auto chunk = rstd_co_try(co_await message_.body.next());
        if (chunk.is_none()) break;
        auto& bytes = *chunk;
        if (bytes.len() > limit - rstd::min(limit, body.len())) {
            message_.body.cancel();
            (void)co_await message_.body.close();
            co_return Err(lifetchto::Error::TooLarge());
        }
        body.extend_from_slice(bytes.as_slice());
    }
    (void)co_await transfer_.control().wait();
    co_return Ok(rstd::move(body));
}

auto HttpResponseStream::close() -> coro<empty> {
    message_.body.cancel();
    (void)co_await message_.body.close();
    (void)co_await transfer_.control().wait();
    co_return empty {};
}

HttpClient::HttpClient(Arc<lifetchto::Engine> engine, lifetchto::http::Client normal,
                       lifetchto::http::Client stream, CookieSession jar)
    : engine_(rstd::move(engine)),
      clients_(Arc<Mutex<Clients>>::make(rstd::move(normal), rstd::move(stream))),
      cookies_(Arc<Mutex<CookieSession>>::make(rstd::move(jar))) {}

auto HttpClient::make_client(const lifetchto::Engine& engine, const network::ClientPolicy& policy,
                             bool streaming) -> Result<lifetchto::http::Client, lifetchto::Error> {
    lifetchto::http::TransportOptions transport;
    transport.proxy = policy.proxy();
    auto backend =
        rstd_try(lifetchto::http::NativeBackend::make(engine.resources(), rstd::move(transport)));
    lifetchto::http::ClientConfig config;
    config.policy.redirect                     = Some(lifetchto::http::RedirectPolicy {});
    config.policy.redirect->allow_cross_origin = true;
    if (streaming) config.total_timeout = None();
    return lifetchto::http::Client::make(engine.handle(),
                                         lifetchto::http::BackendBinding::Owned(backend.backend()),
                                         rstd::move(config));
}

auto HttpClient::client(bool streaming) -> Result<lifetchto::http::Client, lifetchto::Error> {
    Option<tuple<lifetchto::http::Client, lifetchto::http::Client>> retired;
    auto clients = clients_->lock().unwrap();
    if (clients->closed) return Err(lifetchto::Error::Closed());
    auto policy = network::policy(clients->scope);
    if (! clients->policy.same_as(policy)) {
        auto normal     = rstd_try(make_client(*engine_, policy, false));
        auto stream     = rstd_try(make_client(*engine_, policy, true));
        retired         = Some(tuple { rstd::move(clients->normal), rstd::move(clients->stream) });
        clients->normal = rstd::move(normal);
        clients->stream = rstd::move(stream);
        clients->policy = rstd::move(policy);
    }
    return Ok((streaming ? clients->stream : clients->normal).clone());
}

auto HttpClient::make(RuntimeHandle runtime) -> Result<HttpClient, lifetchto::Error> {
    auto created = lifetchto::Engine::make(rstd::move(runtime));
    if (created.is_err()) return Err(rstd::move(created).unwrap_err());
    auto engine = Arc<lifetchto::Engine>::make(rstd::move(created).unwrap());
    auto jar    = CookieSession::make();
    if (jar.is_err())
        return Err(lifetchto::Error::InvalidOptions(String::make("invalid cookie jar"_str)));
    auto normal = rstd_try(make_client(*engine, network::ClientPolicy {}, false));
    auto stream = rstd_try(make_client(*engine, network::ClientPolicy {}, true));
    return Ok(HttpClient {
        rstd::move(engine), rstd::move(normal), rstd::move(stream), rstd::move(jar).unwrap() });
}

auto HttpClient::set_scope(Option<i64> scope) -> void {
    auto clients   = clients_->lock().unwrap();
    clients->scope = scope;
}

auto HttpClient::cookie_session() const -> CookieSession {
    auto session = cookies_->lock().unwrap();
    return session->clone();
}

auto HttpClient::reset_session() -> lifetchto::Result<empty> {
    auto next = CookieSession::make();
    if (next.is_err()) return Err(rstd::move(next).unwrap_err());
    auto session = cookies_->lock().unwrap();
    session->invalidate();
    *session = rstd::move(next).unwrap();
    return Ok(empty {});
}

auto HttpClient::invalidate_session() -> void {
    auto session = cookies_->lock().unwrap();
    session->invalidate();
}

auto HttpClient::get(ref<str> url, usize limit) -> coro<Result<String, lifetchto::Error>> {
    auto selected = rstd_co_try(client(false));
    auto parsed   = rstd_co_try(lihttpto::Url::parse_http(url),
                                lifetchto::Error::InvalidOptions(String::make("invalid url"_str)));
    auto method = rstd_co_try(lihttpto::Method::parse("GET"_str),
                              lifetchto::Error::InvalidOptions(String::make("invalid method"_str)));
    auto request = lifetchto::http::ExecutionRequest {
        lihttpto::RequestParts { rstd::move(parsed), rstd::move(method), lihttpto::Headers {} },
        lifetchto::http::UploadPlan {}
    };
    lifetchto::http::ExecuteOptions options;
    options.cookies = lifetchto::http::RequestCookies::Shared(cookie_session().jar());
    auto submitted  = selected.execute(rstd::move(request), rstd::move(options));
    if (submitted.is_err()) {
        auto rejected = rstd::move(submitted).unwrap_err();
        (void)co_await rejected.input.body.close();
        co_return Err(rstd::move(rejected.error));
    }
    auto    transfer = rstd::move(submitted).unwrap();
    auto    message  = rstd_co_try(co_await transfer.output());
    Vec<u8> body;
    for (;;) {
        auto chunk = rstd_co_try(co_await message.body.next());
        if (chunk.is_none()) break;
        auto& bytes = *chunk;
        if (bytes.len() > limit - rstd::min(limit, body.len()))
            co_return Err(lifetchto::Error::TooLarge());
        body.extend_from_slice(bytes.as_slice());
    }
    (void)co_await transfer.control().wait();
    auto text = rstd_co_try(rstd::str_::from_utf8(body.as_slice()),
                            lifetchto::Error::BodySource(String::make("invalid UTF-8"_str)));
    co_return Ok(String::make(text));
}

auto HttpClient::open(ref<str> method_text, ref<str> url, lihttpto::Headers headers,
                      Option<Vec<u8>> body, Option<Duration> timeout)
    -> coro<Result<HttpResponseStream, lifetchto::Error>> {
    return open_session(
        method_text, url, rstd::move(headers), rstd::move(body), timeout, cookie_session());
}

auto HttpClient::open_request(lifetchto::http::Client selected, lihttpto::RequestParts parts,
                              lifetchto::http::UploadPlan upload, Option<Duration> timeout,
                              CookieSession session, bool streaming)
    -> coro<Result<HttpResponseStream, lifetchto::Error>> {
    auto original_url = String::make(parts.url.as_ref());
    apply_cookies(session, parts.url, parts.headers);
    auto request = lifetchto::http::ExecutionRequest { rstd::move(parts), rstd::move(upload) };
    lifetchto::http::ExecuteOptions options;
    options.cookies = lifetchto::http::RequestCookies::Shared(session.jar());
    lifetchto::http::ExecutionObserver observer;
    options.report = Some(observer.clone());
    if (streaming)
        options.total_timeout = Some(lifetchto::http::TotalTimeout::disabled());
    else if (timeout.is_some())
        options.total_timeout = Some(lifetchto::http::TotalTimeout::after(*timeout));
    auto submitted = selected.execute(rstd::move(request), rstd::move(options));
    if (submitted.is_err()) {
        auto rejected = rstd::move(submitted).unwrap_err();
        (void)co_await rejected.input.body.close();
        co_return Err(rstd::move(rejected.error));
    }
    auto transfer     = rstd::move(submitted).unwrap();
    auto response     = rstd_co_try(co_await transfer.output());
    auto facts        = observer.snapshot();
    auto response_url = facts.request.is_some() ? String::make(facts.request->url.as_ref())
                                                : rstd::move(original_url);
    co_return Ok(HttpResponseStream { rstd::move(selected),
                                      rstd::move(transfer),
                                      rstd::move(response),
                                      rstd::move(response_url) });
}

auto HttpClient::read_response(HttpResponseStream stream, usize limit)
    -> coro<Result<HttpResponse, lifetchto::Error>> {
    auto result    = HttpResponse {};
    result.status  = stream.head().status.value();
    result.headers = stream.head().headers.clone();
    result.url     = String::make(stream.url());
    result.body    = rstd_co_try(co_await stream.read(limit));
    co_return Ok(rstd::move(result));
}

auto HttpClient::request(ref<str> method_text, ref<str> url, lihttpto::Headers headers,
                         Option<Vec<u8>> body, usize limit, Option<Duration> timeout)
    -> coro<Result<HttpResponse, lifetchto::Error>> {
    return request_session(
        method_text, url, rstd::move(headers), rstd::move(body), limit, timeout, cookie_session());
}

auto HttpClient::cookie(ref<str> url, ref<str> name) -> Option<String> {
    auto jar    = cookie_session();
    auto parsed = lihttpto::Url::parse(url);
    if (parsed.is_err()) return None();
    auto value  = SystemTime::now().duration_since(SystemTime::unix_epoch());
    auto now    = value.is_ok() ? i64(value->as_secs().to_primitive()) : i64();
    auto header = jar.select(*parsed, now);
    if (header.is_err()) return None();
    auto found = header->get(name);
    if (found.is_none()) return None();
    return Some(String::make((*found)->value()));
}

auto HttpClient::set_cookie(ref<str> url, ref<str> cookie) -> bool {
    auto jar    = cookie_session();
    auto parsed = lihttpto::Url::parse(url);
    if (parsed.is_err()) return false;
    auto value = lihttpto::SetCookie::parse(cookie);
    if (value.is_err()) return false;
    auto current = SystemTime::now().duration_since(SystemTime::unix_epoch());
    auto now     = current.is_ok() ? i64(current->as_secs().to_primitive()) : i64();
    return jar.store(*parsed, *value, now).is_ok();
}

auto HttpClient::clear_cookies() -> void { (void)cookie_session().clear(); }

auto HttpClient::apply_cookies(const CookieSession& jar, const lihttpto::Url& url,
                               lihttpto::Headers& headers) -> void {
    auto current = headers.get_unique_text("cookie"_str);
    if (current.is_err() || current->is_none()) return;
    auto value    = SystemTime::now().duration_since(SystemTime::unix_epoch());
    auto now      = value.is_ok() ? i64(value->as_secs().to_primitive()) : i64();
    auto selected = jar.select(url, now);
    if (selected.is_err() || selected->is_empty()) return;
    auto combined = String::make(*(*current));
    combined.push_str("; "_str);
    combined.push_str(selected->encode().as_str());
    (void)headers.set("Cookie"_str, combined.as_str());
}

auto HttpClient::load_cookies(ref<str> data) -> bool {
    if (data.is_empty()) return false;
    auto  jar = cookie_session();
    usize start {};
    while (start < data.len()) {
        auto ch = data.as_bytes()[start];
        if (ch != u8(' ') && ch != u8('\n') && ch != u8('\r') && ch != u8('\t')) break;
        ++start;
    }
    if (start < data.len() && data.as_bytes()[start] == u8('[')) {
        auto decoded = decode_cookie_snapshot(data);
        return decoded.is_some() && jar.replace_snapshot(rstd::move(*decoded)).is_ok();
    }
    auto current   = SystemTime::now().duration_since(SystemTime::unix_epoch());
    auto now       = current.is_ok() ? i64(current->as_secs().to_primitive()) : i64();
    auto temporary = CookieSession::make();
    if (temporary.is_err() || temporary->import_netscape(data, now).is_err()) return false;
    auto snapshot = temporary->snapshot_all();
    return snapshot.is_ok() && jar.replace_snapshot(rstd::move(*snapshot)).is_ok();
}

auto HttpClient::save_cookies() -> Result<String, lifetchto::Error> {
    auto encoded = rstd_try(encode_cookie_snapshot(cookie_session()));
    return Ok(rstd::move(encoded.text));
}

auto HttpClient::close() -> coro<empty> {
    Option<tuple<lifetchto::http::Client, lifetchto::http::Client>> clients;
    {
        auto current    = clients_->lock().unwrap();
        current->closed = true;
        clients         = Some(tuple { current->normal.clone(), current->stream.clone() });
    }
    invalidate_session();
    (void)co_await rstd::get<0>(*clients).close();
    (void)co_await rstd::get<1>(*clients).close();
    (void)co_await engine_->close();
    co_return empty {};
}

auto HttpClient::open_session(ref<str> method_text, ref<str> url, lihttpto::Headers headers,
                              Option<Vec<u8>> body, Option<Duration> timeout, CookieSession session,
                              bool streaming)
    -> coro<Result<HttpResponseStream, lifetchto::Error>> {
    if (! session.active()) co_return Err(lifetchto::Error::Canceled());
    auto selected = rstd_co_try(client(streaming));
    auto parsed   = rstd_co_try(lihttpto::Url::parse_http(url),
                                lifetchto::Error::InvalidOptions(String::make("invalid url"_str)));
    auto method = rstd_co_try(lihttpto::Method::parse(method_text),
                              lifetchto::Error::InvalidOptions(String::make("invalid method"_str)));
    auto plan   = lifetchto::http::UploadPlan::empty();
    if (body.is_some())
        plan = lifetchto::http::UploadPlan::bytes(Bytes::copy_from_slice(body->as_slice()));
    co_return co_await open_request(
        rstd::move(selected),
        lihttpto::RequestParts { rstd::move(parsed), rstd::move(method), rstd::move(headers) },
        rstd::move(plan),
        timeout,
        rstd::move(session),
        streaming);
}

auto HttpClient::open_session(lihttpto::RequestParts parts, Option<Duration> timeout,
                              CookieSession session, bool streaming)
    -> coro<Result<HttpResponseStream, lifetchto::Error>> {
    if (! session.active()) co_return Err(lifetchto::Error::Canceled());
    auto selected = rstd_co_try(client(streaming));
    co_return co_await open_request(rstd::move(selected),
                                    rstd::move(parts),
                                    lifetchto::http::UploadPlan::empty(),
                                    timeout,
                                    rstd::move(session),
                                    streaming);
}

auto HttpClient::request_session(ref<str> method_text, ref<str> url, lihttpto::Headers headers,
                                 Option<Vec<u8>> body, usize limit, Option<Duration> timeout,
                                 CookieSession session)
    -> coro<Result<HttpResponse, lifetchto::Error>> {
    auto stream = rstd_co_try(co_await open_session(
        method_text, url, rstd::move(headers), rstd::move(body), timeout, rstd::move(session)));
    co_return co_await read_response(rstd::move(stream), limit);
}

auto HttpClient::request_session(lihttpto::RequestParts parts, usize limit,
                                 Option<Duration> timeout, CookieSession session)
    -> coro<Result<HttpResponse, lifetchto::Error>> {
    auto stream =
        rstd_co_try(co_await open_session(rstd::move(parts), timeout, rstd::move(session)));
    co_return co_await read_response(rstd::move(stream), limit);
}
} // namespace qcm::backend
