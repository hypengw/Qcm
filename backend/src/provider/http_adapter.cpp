module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
module qcm.backend;
import :provider.http_adapter;
import :provider.lua_error;
using rstd::async::coro;
using rstd::bytes::Bytes;
using rstd::sync::Arc;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::lua_http
{
auto invalid_session() -> luato::Error {
    return luato::Error::structured(luato::ErrorKind::Runtime,
                                    "qcm.inner"_Str,
                                    "provider session is no longer valid"_Str,
                                    "provider"_Str,
                                    "not_auth"_Str)
        .with_cause(ProviderError::NotAuth());
}

auto http_error(lifetchto::Error error) -> luato::Error {
    auto code = [&]() -> ref<str> {
        RSTD_MATCH(error) {
            RSTD_CASE(Timeout) { return "timeout"_str; }
            RSTD_CASE(Connect, cause) { return "connect"_str; }
            RSTD_CASE(Resolve, cause) { return "connect"_str; }
            RSTD_CASE(TooLarge) { return "limit"_str; }
            RSTD_CASE_DEFAULT { return "transport"_str; }
        }
    }();
    return luato::Error::structured(luato::ErrorKind::Runtime,
                                    "qcm.http"_Str,
                                    rstd::format("{}", error),
                                    "http"_Str,
                                    String::make(code))
        .with_cause(ProviderError::request(rstd::move(error)));
}

auto from_network(HttpResponse response) -> http::ResponseData {
    return { response.status,
             rstd::move(response.body),
             rstd::move(response.headers),
             rstd::move(response.url) };
}

auto Transport::check() const -> luato::Result<empty> {
    if (! generation.is_current()) return Err(invalid_session());
    return Ok(empty {});
}

auto Transport::send(http::RequestData request, usize limit) const
    -> coro<luato::Result<http::ResponseData>> {
    rstd_co_try(check());
    if (session.is_none()) co_return Err(http::error("http client is unavailable"_str));
    if (! session->cookies.active()) co_return Err(invalid_session());
    auto response = co_await session->bridge->request_session_async(request.method.as_str(),
                                                                    request.url.as_str(),
                                                                    rstd::move(request.headers),
                                                                    rstd::move(request.body),
                                                                    limit,
                                                                    request.timeout,
                                                                    session->cookies.clone());
    rstd_co_try(check());
    if (! session->cookies.active()) co_return Err(invalid_session());
    co_return Ok(from_network(rstd_co_try(rstd::move(response), http_error)));
}

auto Transport::runtime() const -> luato::Result<rstd::async::RuntimeHandle> {
    rstd_try(check());
    if (session.is_none()) return Err(http::error("http client is unavailable"_str));
    return Ok(session->bridge->runtime());
}

auto Transport::cookie(ref<str> url, ref<str> name) const -> Option<String> {
    if (session.is_none()) return None();
    return session->bridge->cookie(url, name);
}

auto Transport::set_cookie(ref<str> url, ref<str> value) const -> bool {
    if (session.is_none()) return false;
    return session->bridge->set_cookie(url, value);
}

auto Transport::clear_cookies() const -> void {
    if (session.is_some()) session->bridge->clear_cookies();
}

Body::State::State(HttpResponseStream stream, CookieSession session, Arc<LuaHttpBridge> owner,
                   ProviderGenerationCheck generation)
    : stream(rstd::move(stream)),
      session(rstd::move(session)),
      owner(rstd::move(owner)),
      cancellation(this->session.watch(this->stream.control())),
      generation(rstd::move(generation)) {}

Body::State::~State() { stream.cancel(); }

Body::Body(HttpResponseStream stream, CookieSession session, Arc<LuaHttpBridge> owner,
           ProviderGenerationCheck generation)
    : state_(Arc<State>::make(rstd::move(stream), rstd::move(session), rstd::move(owner),
                              rstd::move(generation))) {}

auto Body::check() const -> luato::Result<empty> {
    if (! state_->session.active() || ! state_->generation.is_current())
        return Err(invalid_session());
    return Ok(empty {});
}

auto Body::next() const -> coro<luato::Result<Option<Bytes>>> {
    rstd_co_try(check());
    auto result = co_await state_->stream.next();
    rstd_co_try(check());
    co_return Ok(rstd_co_try(rstd::move(result), http_error));
}

auto Body::read(usize limit, Duration timeout) const -> coro<luato::Result<Vec<u8>>> {
    rstd_co_try(check());
    auto result = rstd_co_try(co_await rstd::async::timeout(
                                  rstd::async::into_future(state_->stream.read(limit)), timeout),
                              http_error(lifetchto::Error::Timeout()));
    rstd_co_try(check());
    co_return Ok(rstd_co_try(rstd::move(result), http_error));
}

auto Body::cancel() const -> void { state_->stream.cancel(); }

auto Body::close() const -> coro<empty> {
    co_await state_->stream.close();
    state_->cancellation.reset();
    co_return empty {};
}
} // namespace qcm::backend::lua_http
