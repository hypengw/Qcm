module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
export module qcm.backend:provider.lua.support;
export import luato;
export import luato.extra;
export import rstd;
export import :provider.http_adapter;
export import :provider.session;
export import :provider.common;
export import :resource.model;
export import :plugin.context;
import :playing;
using alloc::rc::Rc;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct BindingsContext {
    Arc<ProviderCommonData> common;
    explicit BindingsContext(Arc<ProviderCommonData> data): common(rstd::move(data)) {}
    Arc<PlayingState>                       playing { Arc<PlayingState>::make() };
    Option<Arc<LuaHttpBridge>>              http;
    Option<Rc<luato::extra::JsonContext>>   json;
    Option<Rc<luato::extra::http::Context>> http_bindings;
    Option<PluginContext>                   installation;
    ProviderGeneration                      generation;
    Arc<ProviderSessionState>               session { Arc<ProviderSessionState>::make() };
    Option<luato::FunctionHandle>           session_save;
    struct PreparedResource {
        resource_cache::PreparedRequest request;
        u64                             generation {};

        auto clone() const -> PreparedResource { return { request.clone(), generation }; }
    };
};
using BindingsContextArc = Arc<BindingsContext>;

namespace bindings
{
class HttpSource final : public luato::extra::http::ClientSource {
    rstd::sync::Weak<BindingsContext> context_;

public:
    explicit HttpSource(rstd::sync::Weak<BindingsContext> context): context_(rstd::move(context)) {}
    auto current() -> luato::Result<luato::extra::http::TransportHandle> override {
        auto owner = context_.upgrade();
        if (! owner) return Err(lua_http::invalid_session());
        auto& context = *owner;
        if (! context.session->active()) return Err(lua_http::invalid_session());
        Option<lua_http::Transport::Session> session;
        if (context.http.is_some()) {
            auto bridge  = context.http->clone();
            auto cookies = bridge->cookie_session();
            session =
                Some(lua_http::Transport::Session { rstd::move(bridge), rstd::move(cookies) });
        }
        return Ok(luato::extra::http::TransportHandle::make(
            lua_http::Transport { rstd::move(session), context.generation.snapshot() }));
    }
};
using lua_http::http_error;
using luato::extra::json_error;
using luato::extra::lower_ascii;
using luato::extra::new_table;
using luato::extra::percent_encode_bytes;
using luato::extra::return_table;
using luato::extra::str_key;
using luato::extra::str_value;
using luato::extra::string_arg;
using luato::extra::table_arg;
using luato::extra::take;
inline auto fail(ref<str> message) -> luato::Error {
    return luato::Error::make(
        luato::ErrorKind::Runtime, String::make("qcm"_str), String::make(message));
}
inline auto invalid_session() -> luato::Error {
    return luato::Error::structured(luato::ErrorKind::Runtime,
                                    String::make("qcm.inner"_str),
                                    String::make("provider session is no longer valid"_str),
                                    String::make("provider"_str),
                                    String::make("not_auth"_str))
        .with_cause(ProviderError::NotAuth());
}

} // namespace bindings
} // namespace qcm::backend
