module;
#include <rstd/macro.hpp>
export module qcm.backend:provider.http_bridge;
export import rstd;
export import :network;
using rstd::async::JoinHandle;
using rstd::async::RuntimeHandle;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
class LuaHttpBridge {
    RuntimeHandle runtime_;
    HttpClient    client_;

    LuaHttpBridge(RuntimeHandle runtime, HttpClient client)
        : runtime_(rstd::move(runtime)), client_(rstd::move(client)) {}

public:
    LuaHttpBridge(const LuaHttpBridge&)                    = delete;
    auto operator=(const LuaHttpBridge&) -> LuaHttpBridge& = delete;
    LuaHttpBridge(LuaHttpBridge&&)                         = default;
    auto operator=(LuaHttpBridge&&) -> LuaHttpBridge&      = default;

    static auto make(RuntimeHandle runtime) -> Result<LuaHttpBridge, lifetchto::Error> {
        auto client = rstd_try(HttpClient::make(runtime.clone()));
        return Ok(LuaHttpBridge(rstd::move(runtime), rstd::move(client)));
    }
    auto set_scope(Option<i64> scope) -> void { client_.set_scope(scope); }
    auto request_async(ref<str> method, ref<str> url, lihttpto::Headers headers,
                       Option<Vec<u8>> body, usize limit, Option<Duration> timeout = None())
        -> rstd::async::coro<Result<HttpResponse, lifetchto::Error>> {
        return client_.request(method, url, rstd::move(headers), rstd::move(body), limit, timeout);
    }
    auto request_session_async(ref<str> method, ref<str> url, lihttpto::Headers headers,
                               Option<Vec<u8>> body, usize limit, Option<Duration> timeout,
                               CookieSession session)
        -> rstd::async::coro<Result<HttpResponse, lifetchto::Error>> {
        return client_.request_session(method,
                                       url,
                                       rstd::move(headers),
                                       rstd::move(body),
                                       limit,
                                       timeout,
                                       rstd::move(session));
    }
    auto open_session_async(ref<str> method, ref<str> url, lihttpto::Headers headers,
                            Option<Vec<u8>> body, Option<Duration> timeout, CookieSession session)
        -> rstd::async::coro<Result<HttpResponseStream, lifetchto::Error>> {
        auto deadline = timeout.is_some() ? *timeout : Duration::from_secs(u64(30));
        auto opened =
            rstd_co_try(co_await rstd::async::timeout(
                            rstd::async::into_future(client_.open_session(method,
                                                                          url,
                                                                          rstd::move(headers),
                                                                          rstd::move(body),
                                                                          None(),
                                                                          rstd::move(session),
                                                                          true)),
                            deadline),
                        lifetchto::Error::Timeout());
        co_return rstd::move(opened);
    }
    auto request_prepared_async(lihttpto::RequestParts request, usize limit,
                                Option<Duration> timeout, CookieSession session)
        -> rstd::async::coro<Result<HttpResponse, lifetchto::Error>> {
        return client_.request_session(rstd::move(request), limit, timeout, rstd::move(session));
    }
    auto open_prepared_async(lihttpto::RequestParts request, Option<Duration> timeout,
                             CookieSession session)
        -> rstd::async::coro<Result<HttpResponseStream, lifetchto::Error>> {
        auto deadline = timeout.is_some() ? *timeout : Duration::from_secs(u64(30));
        auto opened   = rstd_co_try(
            co_await rstd::async::timeout(
                client_.open_session(rstd::move(request), None(), rstd::move(session), true)
                    .into_future(),
                deadline),
            lifetchto::Error::Timeout());
        co_return rstd::move(opened);
    }
    auto cookie_session() const -> CookieSession { return client_.cookie_session(); }
    auto runtime() const -> RuntimeHandle { return runtime_.clone(); }
    auto reset_session() -> lifetchto::Result<empty> { return client_.reset_session(); }
    auto invalidate_session() -> void { client_.invalidate_session(); }
    auto cancel_streams() -> void { client_.cookie_session().cancel_streams(); }
    auto cookie(ref<str> url, ref<str> name) -> Option<String> { return client_.cookie(url, name); }
    auto set_cookie(ref<str> url, ref<str> cookie) -> bool {
        return client_.set_cookie(url, cookie);
    }
    auto clear_cookies() -> void { client_.clear_cookies(); }
    auto load_cookies(ref<str> data) -> bool { return client_.load_cookies(data); }
    auto save_cookies() -> Result<String, lifetchto::Error> { return client_.save_cookies(); }
    template<class A>
    auto spawn(A awaitable) -> JoinHandle<rstd::async::await_output_t<A>> {
        return runtime_.spawn(rstd::move(awaitable));
    }
    auto close() -> rstd::async::coro<empty> {
        (void)co_await client_.close();
        co_return empty {};
    }
};
} // namespace qcm::backend
