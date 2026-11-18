module;
#include <rstd/enum.hpp>
module qcm.backend;
import :sync_state;
using rstd::sync::Arc;
using namespace rstd::prelude;

namespace qcm::backend
{
auto sync_state_from_cause(rstd::error::ErrorRef error) -> SyncState;
auto sync_state_from_provider_error(const ProviderError& error) -> SyncState {
    RSTD_MATCH(error.root()) {
        RSTD_CASE(NotAuth) { return SyncState::NotAuth; }
        RSTD_CASE(AuthRequired, message, jump_url) { return SyncState::NotAuth; }
        RSTD_CASE(Request, cause) { return SyncState::NetworkError; }
        RSTD_CASE(Db, cause) { return SyncState::DatabaseError; }
        RSTD_CASE(Io, cause) { return SyncState::IoError; }
        RSTD_CASE(External, cause) {
            const auto& value = *cause;
            return sync_state_from_cause(dyn<rstd::error::Error>::from_ref(value));
        }
        RSTD_CASE_DEFAULT { return SyncState::UnknownError; }
    }
}
auto sync_state_from_cause(rstd::error::ErrorRef error) -> SyncState {
    for (;;) {
        if (auto value = rstd::error::downcast_ref<ProviderError>(error))
            return sync_state_from_provider_error(**value);
        if (auto value = rstd::error::downcast_ref<Arc<ProviderError>>(error))
            return sync_state_from_provider_error(***value);
        if (rstd::error::is<sqlito::Error>(error)) return SyncState::DatabaseError;
        if (rstd::error::is<rstd::io::Error>(error)) return SyncState::IoError;
        if (rstd::error::is<lifetchto::Error>(error)) return SyncState::NetworkError;
        auto next = error->source();
        if (next.is_none()) return SyncState::UnknownError;
        error = *next;
    }
}
} // namespace qcm::backend
