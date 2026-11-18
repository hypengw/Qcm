module;
#include <rstd/macro.hpp>
module qcm.backend;
import :provider.http_bridge;
using rstd::async::RuntimeHandle;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
auto LuaHttpBridge::make(RuntimeHandle runtime) -> Result<LuaHttpBridge, lifetchto::Error> {
    auto client = rstd_try(HttpClient::make(runtime.clone()));
    return Ok(LuaHttpBridge(rstd::move(runtime), rstd::move(client)));
}

auto LuaHttpBridge::open_session_async(ref<str> method, ref<str> url, lihttpto::Headers headers,
                                       Option<Vec<u8>> body, Option<Duration> timeout,
                                       CookieSession session)
    -> rstd::async::coro<Result<HttpResponseStream, lifetchto::Error>> {
    auto deadline = timeout.is_some() ? *timeout : Duration::from_secs(u64(30));
    auto opened = rstd_co_try(co_await rstd::async::timeout(
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

auto LuaHttpBridge::open_prepared_async(lihttpto::RequestParts request, Option<Duration> timeout,
                                        CookieSession session)
    -> rstd::async::coro<Result<HttpResponseStream, lifetchto::Error>> {
    auto deadline = timeout.is_some() ? *timeout : Duration::from_secs(u64(30));
    auto opened =
        rstd_co_try(co_await rstd::async::timeout(
                        client_.open_session(rstd::move(request), None(), rstd::move(session), true)
                            .into_future(),
                        deadline),
                    lifetchto::Error::Timeout());
    co_return rstd::move(opened);
}

auto LuaHttpBridge::close() -> rstd::async::coro<empty> {
    (void)co_await client_.close();
    co_return empty {};
}
} // namespace qcm::backend
