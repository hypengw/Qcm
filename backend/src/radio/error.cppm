module;
#include <rstd/enum.hpp>
export module qcm.backend:radio.error;
export import :provider.error;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;
export namespace qcm::backend
{
// clang-format off
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Debug, Error)]] enum RadioError {
        [[error("Radio session expired; restart the radio to continue")]]
        Expired,
        [[error("Radio session version or operation conflict")]]
        Conflict,
        [[error("{message}")]]
        Invalid(message : String),
        [[error("{cause}")]]
        Provider([[from]] cause : Arc<ProviderError>),
    } members {
    public:
        RadioError(ProviderError error);
        auto clone() const -> RadioError;
    })]];
// clang-format on

inline auto RadioError::clone() const -> RadioError {
    RSTD_MATCH(*this) {
        RSTD_CASE(Expired) { return Expired(); }
        RSTD_CASE(Conflict) { return Conflict(); }
        RSTD_CASE(Invalid, message) { return Invalid(message.clone()); }
        RSTD_CASE(Provider, cause) { return Provider(cause.clone()); }
    }
}
inline RadioError::RadioError(ProviderError error)
    : RadioError(Provider(Arc<ProviderError>::make(rstd::move(error)))) {}
} // namespace qcm::backend
export namespace rstd
{
template<>
struct Impl<convert::From<qcm::backend::ProviderError>, qcm::backend::RadioError> {
    static auto from(qcm::backend::ProviderError error) -> qcm::backend::RadioError {
        return qcm::backend::RadioError(rstd::move(error));
    }
};
} // namespace rstd
