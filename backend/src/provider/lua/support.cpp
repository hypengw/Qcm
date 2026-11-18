module qcm.backend;
import :provider.lua.support;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
BindingsContext::BindingsContext(Arc<ProviderCommonData> data): common(rstd::move(data)) {}
} // namespace qcm::backend

namespace qcm::backend::bindings
{
HttpSource::HttpSource(rstd::sync::Weak<BindingsContext> context): context_(rstd::move(context)) {}

auto HttpSource::current() -> luato::Result<luato::extra::http::TransportHandle> {
    auto owner = context_.upgrade();
    if (! owner) return Err(lua_http::invalid_session());
    auto& context = *owner;
    if (! context.session->active()) return Err(lua_http::invalid_session());
    Option<lua_http::Transport::Session> session;
    if (context.http.is_some()) {
        auto bridge  = context.http->clone();
        auto cookies = bridge->cookie_session();
        session = Some(lua_http::Transport::Session { rstd::move(bridge), rstd::move(cookies) });
    }
    return Ok(luato::extra::http::TransportHandle::make(
        lua_http::Transport { rstd::move(session), context.generation.snapshot() }));
}

auto fail(ref<str> message) -> luato::Error {
    return luato::Error::make(
        luato::ErrorKind::Runtime, String::make("qcm"_str), String::make(message));
}

auto invalid_session() -> luato::Error {
    return luato::Error::structured(luato::ErrorKind::Runtime,
                                    String::make("qcm.inner"_str),
                                    String::make("provider session is no longer valid"_str),
                                    String::make("provider"_str),
                                    String::make("not_auth"_str))
        .with_cause(ProviderError::NotAuth());
}
} // namespace qcm::backend::bindings
