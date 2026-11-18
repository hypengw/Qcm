module;
#include <rstd/enum.hpp>
module qcm.backend;
import :resource.error;
using namespace rstd::prelude;
using namespace rstd::literals;
using qcm::backend::ProviderError;
using rstd::sync::Arc;

namespace qcm::backend::resource_cache
{
Error::Error(sqlito::Error cause): Error(storage(rstd::move(cause))) {}

Error::Error(rstd::io::Error cause): Error(io(rstd::move(cause))) {}

Error::Error(ProviderError cause): Error(provider(rstd::move(cause))) {}

auto Error::request(lifetchto::Error cause) -> Error {
    return provider(ProviderError::request(rstd::move(cause)));
}

auto Error::provider(ProviderError cause) -> Error {
    return Provider(Arc<ProviderError>::make(rstd::move(cause)));
}

auto Error::storage(sqlito::Error cause) -> Error {
    return Storage(Arc<sqlito::Error>::make(rstd::move(cause)));
}

auto Error::io(rstd::io::Error cause) -> Error {
    return Io(Arc<rstd::io::Error>::make(rstd::move(cause)));
}

auto Error::invalidated() -> Error { return Invalidated(); }

auto Error::changed() -> Error { return Changed(); }

auto Error::plain_corrupt() -> Error { return PlainCorrupt(); }

auto Error::upstream(i32 status, lihttpto::Headers headers) -> Error {
    return Upstream(status, rstd::move(headers));
}

auto Error::state(ref<str> message) -> Error { return State(String::make(message), None()); }

auto Error::retryable_request() const -> bool {
    RSTD_MATCH(*this) {
        RSTD_CASE(Provider, provider) {
            const ProviderError& error = *provider;
            RSTD_MATCH(error) {
                RSTD_CASE(Request, cause) {
                    return cause.is_Resolve() || cause.is_Connect() || cause.is_Timeout();
                }
                RSTD_CASE_DEFAULT { return false; }
            }
        }
        RSTD_CASE_DEFAULT { return false; }
    }
}

auto Error::should_reopen() const -> bool { return is_Changed() || is_PlainCorrupt(); }

auto Error::clone() const -> Error {
    RSTD_MATCH(*this) {
        RSTD_CASE(Provider, cause) { return Provider(cause.clone()); }
        RSTD_CASE(Storage, cause) { return Storage(cause.clone()); }
        RSTD_CASE(Io, cause) { return Io(cause.clone()); }
        RSTD_CASE(Invalidated) { return Invalidated(); }
        RSTD_CASE(Changed) { return Changed(); }
        RSTD_CASE(PlainCorrupt) { return PlainCorrupt(); }
        RSTD_CASE(Upstream, status, headers) { return Upstream(status, headers.clone()); }
        RSTD_CASE(State, message, cause) {
            return State(message.clone(), cause.is_some() ? Some(cause->clone()) : None());
        }
    }
}
} // namespace qcm::backend::resource_cache
