export module qcm.backend:resource.lifetime;
export import rstd;
using rstd::async::coro;
namespace watch = rstd::async::watch;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
class PlaybackResourceLifetime {
    watch::Sender<bool> cancelled_ { false };

public:
    auto active() const -> bool;
    auto cancelled() -> coro<empty>;
    auto cancel() -> void;
};
using PlaybackLifetime = Arc<PlaybackResourceLifetime>;
} // namespace qcm::backend
