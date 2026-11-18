export module qcm.backend:sync_state;
export import :provider.error;
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
auto sync_state_from_provider_error(const ProviderError& error) -> SyncState;
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
