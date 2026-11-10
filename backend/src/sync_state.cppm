module;
#include <rstd/enum.hpp>
export module qcm.backend:sync_state;
export import :provider.error;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
enum class [[pmacro::derive(
    "rstd::EnumIter", "rstd::TryFrom", "rstd::Debug", "rstd::Display",
    "rstd::FromStr")]] [[pmacro::helper("rstd::FromStr", ascii_case_insensitive)]] SyncState : int
{
    Finished                                                                                = 0,
    Syncing                                                                                 = 1,
    NotAuth                                                                                 = 2,
    NetworkError                                                                            = 3,
    DatabaseError [[pmacro::helper("rstd::Debug", rename("DBError"))]] [[pmacro::helper(
        "rstd::Display", rename("DBError"))]] [[pmacro::helper("rstd::FromStr",
                                                               rename("DBError"))]]         = 4,
    IoError [[pmacro::helper("rstd::Debug", rename("IOError"))]] [[pmacro::helper(
        "rstd::Display", rename("IOError"))]] [[pmacro::helper("rstd::FromStr",
                                                               rename("IOError"))]]         = 5,
    UnknownError [[pmacro::helper("rstd::Debug", rename("UknownError"))]] [[pmacro::helper(
        "rstd::Display", rename("UknownError"))]] [[pmacro::helper("rstd::FromStr",
                                                                   rename("UknownError"))]] = 6
};
} // namespace qcm::backend

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

export namespace rstd
{
template<>
struct Impl<convert::From<qcm::backend::ProviderError>, qcm::backend::SyncState> {
    static auto from(qcm::backend::ProviderError error) -> qcm::backend::SyncState {
        return qcm::backend::sync_state_from_provider_error(error);
    }
};
template<>
struct Impl<convert::From<error::ErrorRef>, qcm::backend::SyncState> {
    static auto from(error::ErrorRef error) -> qcm::backend::SyncState {
        return qcm::backend::sync_state_from_cause(error);
    }
};
} // namespace rstd
