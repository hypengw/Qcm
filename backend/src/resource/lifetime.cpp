module qcm.backend;
import :resource.lifetime;
using rstd::async::coro;
using namespace rstd::prelude;

namespace qcm::backend
{
auto PlaybackResourceLifetime::active() const -> bool { return ! *cancelled_.borrow(); }

auto PlaybackResourceLifetime::cancelled() -> coro<empty> {
    auto receiver = cancelled_.subscribe();
    while (! *receiver.borrow_and_update()) {
        auto changed = co_await receiver.changed();
        if (changed.is_err()) break;
    }
    co_return empty {};
}

auto PlaybackResourceLifetime::cancel() -> void { (void)cancelled_.send_replace(true); }
} // namespace qcm::backend
