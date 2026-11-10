export module qcm.backend:resource.lifetime;
export import :network.cookie_session;
using rstd::async::coro;
namespace watch = rstd::async::watch;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
class PlaybackResourceLifetime {
    watch::Sender<bool> cancelled_ { false };

public:
    auto active() const -> bool { return ! *cancelled_.borrow(); }
    auto cancelled() -> coro<empty> {
        auto receiver = cancelled_.subscribe();
        while (! *receiver.borrow_and_update()) {
            auto changed = co_await receiver.changed();
            if (changed.is_err()) break;
        }
        co_return empty {};
    }
    auto cancel() -> void { (void)cancelled_.send_replace(true); }
};
using PlaybackLifetime = Arc<PlaybackResourceLifetime>;
} // namespace qcm::backend
