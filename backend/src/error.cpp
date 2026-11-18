module;
#include <rstd/enum.hpp>
module qcm.backend;
import :error;
import rstd.log;
using namespace rstd::prelude;
using rstd::sync::Arc;

namespace qcm::backend
{
ProcessError::ProcessError(ProviderError error): ProcessError(from_provider(rstd::move(error))) {}

ProcessError::ProcessError(Arc<ProviderError> error)
    : ProcessError(from_provider(rstd::move(error))) {}

ProcessError::ProcessError(sqlito::Error error): ProcessError(db(rstd::move(error))) {}

ProcessError::ProcessError(resource_cache::Error error)
    : ProcessError(from_resource(rstd::move(error))) {}

ProcessError::ProcessError(RadioError error): ProcessError(radio(rstd::move(error))) {}

ProcessError::ProcessError(liorm::BindError<sqlito::Error> error)
    : ProcessError(binding(rstd::move(error))) {}

ProcessError::ProcessError(QueryError error): ProcessError(query(rstd::move(error))) {}

auto ProcessError::query(QueryError error) -> ProcessError {
    return Db(error_cause(rstd::move(error)));
}

auto ProcessError::binding(liorm::BindError<sqlito::Error> error) -> ProcessError {
    return Db(error_cause(rstd::move(error)));
}

auto ProcessError::internal(ref<str> message) -> ProcessError {
    return Internal(error_cause(message));
}

auto ProcessError::resource(resource_cache::Error error) -> ProcessError {
    return Resource(rstd::move(error));
}

auto ProcessError::from_provider(ProviderError error) -> ProcessError {
    return from_provider(Arc<ProviderError>::make(rstd::move(error)));
}

auto ProcessError::from_provider(Arc<ProviderError> error) -> ProcessError {
    const auto& root = error->root();
    if (error->is_WithContext()) {
        if (root.is_NotFound())
            rstd::log::debug("{}", *error);
        else
            rstd::log::error("{}", *error);
    }
    RSTD_MATCH(root) {
        RSTD_CASE(NotFound) { return NotFound(); }
        RSTD_CASE(NotAuth) { return NotAuth(); }
        RSTD_CASE(AuthRequired, message, jump_url) {
            return AuthRequired(message.clone(), jump_url.clone());
        }
        RSTD_CASE(ParseSubtitle, message) { return ParseSubtitle(message.clone()); }
        RSTD_CASE_DEFAULT { return internal(rstd::move(error)); }
    }
}

auto ProcessError::from_resource(resource_cache::Error error) -> ProcessError {
    return resource(rstd::move(error));
}

auto ProcessError::radio(RadioError error) -> ProcessError {
    RSTD_MATCH(error) {
        RSTD_CASE(Provider, cause) { return from_provider(rstd::move(cause)); }
        RSTD_CASE_DEFAULT { return Radio(rstd::move(error)); }
    }
}
} // namespace qcm::backend

namespace rstd
{
auto Impl<convert::From<qcm::backend::QueryError>, qcm::backend::ProcessError>::from(
    qcm::backend::QueryError error) -> qcm::backend::ProcessError {
    return qcm::backend::ProcessError::query(rstd::move(error));
}

auto Impl<convert::From<liorm::BindError<sqlito::Error>>, qcm::backend::ProcessError>::from(
    liorm::BindError<sqlito::Error> error) -> qcm::backend::ProcessError {
    return qcm::backend::ProcessError::binding(rstd::move(error));
}

auto Impl<convert::From<qcm::backend::ProviderError>, qcm::backend::ProcessError>::from(
    qcm::backend::ProviderError error) -> qcm::backend::ProcessError {
    return qcm::backend::ProcessError(rstd::move(error));
}

auto Impl<convert::From<sync::Arc<qcm::backend::ProviderError>>, qcm::backend::ProcessError>::from(
    sync::Arc<qcm::backend::ProviderError> error) -> qcm::backend::ProcessError {
    return qcm::backend::ProcessError(rstd::move(error));
}

auto Impl<convert::From<sqlito::Error>, qcm::backend::ProcessError>::from(sqlito::Error error)
    -> qcm::backend::ProcessError {
    return qcm::backend::ProcessError(rstd::move(error));
}

auto Impl<convert::From<qcm::backend::resource_cache::Error>, qcm::backend::ProcessError>::from(
    qcm::backend::resource_cache::Error error) -> qcm::backend::ProcessError {
    return qcm::backend::ProcessError(rstd::move(error));
}

auto Impl<convert::From<qcm::backend::RadioError>, qcm::backend::ProcessError>::from(
    qcm::backend::RadioError error) -> qcm::backend::ProcessError {
    return qcm::backend::ProcessError(rstd::move(error));
}
} // namespace rstd
