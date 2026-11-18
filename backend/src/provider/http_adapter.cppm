export module qcm.backend:provider.http_adapter;
export import luato.extra;
export import :provider.http_bridge;
export import :provider.generation;
using rstd::async::coro;
using rstd::bytes::Bytes;
using rstd::sync::Arc;
using rstd::time::Duration;
using namespace rstd::prelude;

export namespace qcm::backend
{
namespace lua_http
{
namespace http = luato::extra::http;
auto invalid_session() -> luato::Error;
auto http_error(lifetchto::Error error) -> luato::Error;
auto from_network(HttpResponse response) -> http::ResponseData;
struct Transport {
    struct Session {
        Arc<LuaHttpBridge> bridge;
        CookieSession      cookies;
    };
    Option<Session>         session;
    ProviderGenerationCheck generation;

    auto check() const -> luato::Result<empty>;
    auto send(http::RequestData request, usize limit) const
        -> coro<luato::Result<http::ResponseData>>;
    auto runtime() const -> luato::Result<rstd::async::RuntimeHandle>;
    auto cookie(ref<str> url, ref<str> name) const -> Option<String>;
    auto set_cookie(ref<str> url, ref<str> value) const -> bool;
    auto clear_cookies() const -> void;
};
class Body {
    struct State {
        HttpResponseStream             stream;
        CookieSession                  session;
        Arc<LuaHttpBridge>             owner;
        Arc<SessionStreamCancellation> cancellation;
        ProviderGenerationCheck        generation;
        State(HttpResponseStream stream, CookieSession session, Arc<LuaHttpBridge> owner,
              ProviderGenerationCheck generation);
        ~State();
    };
    Arc<State> state_;

public:
    Body(HttpResponseStream stream, CookieSession session, Arc<LuaHttpBridge> owner,
         ProviderGenerationCheck generation);
    auto check() const -> luato::Result<empty>;
    auto next() const -> coro<luato::Result<Option<Bytes>>>;
    auto read(usize limit, Duration timeout) const -> coro<luato::Result<Vec<u8>>>;
    auto cancel() const -> void;
    auto close() const -> coro<empty>;
};
} // namespace lua_http
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<luato::extra::http::Transport, qcm::backend::lua_http::Transport>
    : ImplBase<qcm::backend::lua_http::Transport> {
    auto check() const { return self().check(); }
    auto send(luato::extra::http::RequestData request, usize limit) const {
        return self().send(rstd::move(request), limit);
    }
    auto runtime() const { return self().runtime(); }
    auto cookie(ref<str> url, ref<str> name) const { return self().cookie(url, name); }
    auto set_cookie(ref<str> url, ref<str> value) const { return self().set_cookie(url, value); }
    auto clear_cookies() const -> void { self().clear_cookies(); }
};
template<>
struct Impl<luato::extra::http::ResponseBody, qcm::backend::lua_http::Body>
    : ImplBase<qcm::backend::lua_http::Body> {
    auto check() const { return self().check(); }
    auto next() const { return self().next(); }
    auto read(usize limit, Duration timeout) const { return self().read(limit, timeout); }
    auto cancel() const -> void { self().cancel(); }
    auto close() const { return self().close(); }
};
} // namespace rstd
