module;
#include <rstd/macro.hpp>
module qcm.backend;
import :resource.prepared;
using qcm::backend::ProviderError;
using rstd::async::coro;
using rstd::bytes::Bytes;
using rstd::sync::Arc;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::resource_cache
{
auto ResourceValidity::poll_invalidated(CookieSession session, Arc<Check> current) -> coro<empty> {
    while (session.active() && (*current)->operator()())
        co_await rstd::async::sleep(Duration::from_millis(u64(100)));
    co_return empty {};
}

auto ResourceValidity::wait(CookieSession session, Arc<Check> current,
                            Option<PlaybackLifetime> lifetime) -> coro<empty> {
    if (lifetime) {
        (void)co_await rstd::async::select(
            (*lifetime)->cancelled().into_future(),
            poll_invalidated(rstd::move(session), rstd::move(current)).into_future());
    } else {
        co_await poll_invalidated(rstd::move(session), rstd::move(current));
    }
    co_return empty {};
}

ResourceValidity::ResourceValidity(CookieSession session, Option<PlaybackLifetime> lifetime,
                                   Arc<Check> current)
    : session_(rstd::move(session)),
      lifetime_(rstd::move(lifetime)),
      current_(rstd::move(current)) {}

auto ResourceValidity::is_valid() const -> bool {
    return (*current_)->operator()() && session_.active() &&
           (! lifetime_ || (*lifetime_)->active());
}

auto ResourceValidity::with_lifetime(Option<PlaybackLifetime> lifetime) const
    -> Arc<ResourceValidity> {
    return Arc<ResourceValidity>::make(session_.clone(), rstd::move(lifetime), current_.clone());
}

auto ResourceValidity::cancelled() const -> coro<empty> {
    return wait(session_.clone(), current_.clone(), lifetime_.clone());
}

struct ValidatedResourceBody {
    ResourceBody          body;
    Arc<ResourceValidity> validity;
    void                  cancel() { body.cancel(); }
    auto                  close() -> coro<empty> { return body.close(); }
    auto                  next() -> coro<Result<Option<Bytes>, Error>> {
        if (! validity->is_valid()) {
            body.cancel();
            co_return Err(Error::invalidated());
        }
        auto selected = co_await rstd::async::select(body.next().into_future(),
                                                     validity->cancelled().into_future());
        if (selected.is_right() || ! validity->is_valid()) {
            body.cancel();
            co_return Err(Error::invalidated());
        }
        co_return rstd::move(selected).unwrap_left();
    }
};

auto ResourceValidity::guard(Response response) const -> Response {
    auto body = Arc<ValidatedResourceBody>::make(rstd::move(response.body),
                                                 with_lifetime(lifetime_.clone()));
    if (! body->validity->is_valid()) body->cancel();
    response.body = resource_body(rstd::move(body));
    return response;
}

PreparedResource::PreparedResource(PreparedRequest request, Arc<Fetch> fetch,
                                   Arc<ResourceValidity> validity)
    : request_(rstd::move(request)), fetch_(rstd::move(fetch)), validity_(rstd::move(validity)) {}

auto PreparedResource::execute_owned(PreparedRequest request, Arc<Fetch> fetch,
                                     Arc<ResourceValidity> validity, bool streaming)
    -> coro<Result<Response, ProviderError>> {
    if (! validity->is_valid()) co_return Err(ProviderError::NotAuth());
    auto selected = co_await rstd::async::select(
        (*fetch)->operator()(rstd::move(request), streaming).into_future(),
        validity->cancelled().into_future());
    if (selected.is_right() || ! validity->is_valid()) co_return Err(ProviderError::NotAuth());
    auto response = rstd_co_try(rstd::move(selected).unwrap_left());
    if (! validity->is_valid()) {
        response.cancel();
        co_return Err(ProviderError::NotAuth());
    }
    co_return Ok(validity->guard(rstd::move(response)));
}

auto PreparedResource::fetch_owned(PreparedRequest request, Arc<Fetch> fetch,
                                   Arc<ResourceValidity> validity, lihttpto::Headers headers)
    -> coro<Result<Response, Error>> {
    request.extend_delivery_headers(headers);
    auto result =
        co_await execute_owned(rstd::move(request), rstd::move(fetch), validity.clone(), true);
    if (! validity->is_valid()) co_return Err(Error::invalidated());
    co_return rstd::move(result).map_err(Error::provider);
}

PreparedResource::PreparedResource(PreparedRequest request, Fetch fetch, CookieSession session,
                                   Option<PlaybackLifetime> lifetime, Check current)
    : request_(rstd::move(request)),
      fetch_(Arc<Fetch>::make(rstd::move(fetch))),
      validity_(Arc<ResourceValidity>::make(rstd::move(session), rstd::move(lifetime),
                                            Arc<Check>::make(rstd::move(current)))) {}

auto PreparedResource::clone() const -> PreparedResource {
    return PreparedResource(request_.clone(), fetch_.clone(), validity_.clone());
}

auto PreparedResource::with_lifetime(Option<PlaybackLifetime> lifetime) && -> PreparedResource {
    if (lifetime.is_some()) validity_ = validity_->with_lifetime(rstd::move(lifetime));
    return rstd::move(*this);
}

auto PreparedResource::apply_headers(const lihttpto::Headers& headers)
    -> Result<empty, ProviderError> {
    if (! is_valid()) return Err(ProviderError::NotAuth());
    return request_.apply_headers(headers);
}

auto PreparedResource::replace_delivery_headers(const lihttpto::Headers& headers)
    -> Result<empty, ProviderError> {
    if (! is_valid()) return Err(ProviderError::NotAuth());
    return request_.replace_delivery_headers(headers);
}

auto PreparedResource::cache_url(ref<str> effective) const -> Result<String, Error> {
    return Ok(rstd_try(request_.cache_url(effective), Error::provider));
}

auto PreparedResource::is_valid() const -> bool { return validity_->is_valid(); }

auto PreparedResource::guard() const -> Arc<ResourceValidity> { return validity_.clone(); }

auto PreparedResource::cancelled() const -> coro<empty> { return validity_->cancelled(); }

auto PreparedResource::execute(bool streaming) && -> coro<Result<Response, ProviderError>> {
    return execute_owned(
        rstd::move(request_), rstd::move(fetch_), rstd::move(validity_), streaming);
}

auto PreparedResource::fetch(lihttpto::Headers headers) const -> coro<Result<Response, Error>> {
    return fetch_owned(request_.clone(), fetch_.clone(), validity_.clone(), rstd::move(headers));
}

PreparedResource::PreparedResource(PreparedRequest request, Fetch fetch, CookieSession session,
                                   Option<PlaybackLifetime> lifetime, bool current)
    : PreparedResource(rstd::move(request), rstd::move(fetch), rstd::move(session),
                       rstd::move(lifetime), Check::make([current] {
                           return current;
                       })) {}
} // namespace qcm::backend::resource_cache
