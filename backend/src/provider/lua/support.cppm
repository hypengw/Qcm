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

export namespace qcm::backend
{
struct BindingsContext {
    Arc<ProviderCommonData> common;
    explicit BindingsContext(Arc<ProviderCommonData> data);
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
    explicit HttpSource(rstd::sync::Weak<BindingsContext> context);
    auto current() -> luato::Result<luato::extra::http::TransportHandle> override;
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
auto fail(ref<str> message) -> luato::Error;
auto invalid_session() -> luato::Error;

} // namespace bindings
} // namespace qcm::backend
