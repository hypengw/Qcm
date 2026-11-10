module;
#include <rstd/macro.hpp>
export module qcm.backend:provider.queue;
export import :provider.error;
export import :model.radio;
import rstd.json;
using namespace rstd::prelude;

export namespace qcm::backend
{
auto legacy_queue_state(const QueueConfiguration& configuration) -> ProviderResult<String> {
    auto result = rstd::json::encode(configuration);
    if (result.is_err()) return Err(ProviderError::serde(rstd::move(result).unwrap_err()));
    return Ok(rstd::move(*result));
}
auto legacy_queue_configuration(ref<str> state) -> ProviderResult<QueueConfiguration> {
    auto decoded = rstd::json::decode<QueueConfiguration>(state);
    if (decoded.is_err()) return Err(ProviderError::serde(rstd::move(decoded).unwrap_err()));
    return Ok(rstd::move(*decoded));
}
} // namespace qcm::backend
