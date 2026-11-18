module qcm.backend;
import :provider.error;
using namespace rstd::prelude;
using rstd::sync::Arc;

namespace qcm::backend
{
ProviderError::ProviderError(sqlito::Error error): ProviderError(db(rstd::move(error))) {}

ProviderError::ProviderError(rstd::io::Error error): ProviderError(io(rstd::move(error))) {}

ProviderError::ProviderError(lifetchto::Error error): ProviderError(request(rstd::move(error))) {}

ProviderError::ProviderError(liorm::BindError<sqlito::Error> error)
    : ProviderError(binding(rstd::move(error))) {}

auto ProviderError::binding(liorm::BindError<sqlito::Error> error) -> ProviderError {
    return db(rstd::move(error));
}

auto ProviderError::internal(ref<str> message) -> ProviderError {
    return Internal(error_cause(message));
}

auto ProviderError::request(lifetchto::Error error) -> ProviderError {
    return Request(rstd::move(error));
}

auto ProviderError::io(rstd::io::Error error) -> ProviderError { return Io(rstd::move(error)); }

auto ProviderError::root() const& -> const ProviderError& {
    auto* error = this;
    while (error->is_WithContext()) error = rstd::addressof(*error->as_WithContext().error);
    return *error;
}

auto ProviderError::with_context(ref<str> context) && -> ProviderError {
    return WithContext(String::make(context), Arc<ProviderError>::make(rstd::move(*this)));
}
} // namespace qcm::backend

namespace rstd
{
auto Impl<convert::From<liorm::BindError<sqlito::Error>>, qcm::backend::ProviderError>::from(
    liorm::BindError<sqlito::Error> error) -> qcm::backend::ProviderError {
    return qcm::backend::ProviderError::binding(rstd::move(error));
}

auto Impl<convert::From<sqlito::Error>, qcm::backend::ProviderError>::from(sqlito::Error error)
    -> qcm::backend::ProviderError {
    return qcm::backend::ProviderError::db(rstd::move(error));
}
} // namespace rstd
