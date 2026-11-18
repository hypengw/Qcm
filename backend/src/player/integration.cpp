module qcm.backend.player.integration;
import rstd;
using rstd::async::coro;
using namespace rstd::prelude;

namespace qcm::backend
{
auto PlayerIntegrations::update_state(PlayerStateSnapshot state) -> coro<PlayerIntegrationResult> {
    Option<String> error;
    for (auto& child : children_) {
        auto result = co_await child->update_state(state.clone());
        if (result.is_err() && error.is_none()) error = Some(rstd::move(result).unwrap_err());
    }
    if (error.is_some()) co_return Err(rstd::move(*error));
    co_return Ok(empty {});
}

auto PlayerIntegrations::update_queue(PlayerQueueSnapshot queue) -> coro<PlayerIntegrationResult> {
    Option<String> error;
    for (auto& child : children_) {
        auto result = co_await child->update_queue(queue.clone());
        if (result.is_err() && error.is_none()) error = Some(rstd::move(result).unwrap_err());
    }
    if (error.is_some()) co_return Err(rstd::move(*error));
    co_return Ok(empty {});
}

auto PlayerIntegrations::shutdown() -> coro<empty> {
    while (! children_.is_empty()) {
        auto child = children_.pop().unwrap();
        co_await child->shutdown();
    }
    co_return empty {};
}
} // namespace qcm::backend
