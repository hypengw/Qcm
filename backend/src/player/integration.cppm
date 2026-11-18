export module qcm.backend.player.integration;
import proto.p_message;
import rstd;

using rstd::async::coro;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
using PlayerIntegrationResult = Result<empty, String>;

template<class T>
class PlayerSnapshot {
    Arc<T> value_;
    explicit PlayerSnapshot(Arc<T> value): value_(rstd::move(value)) {}

public:
    static auto make(T value = {}) -> PlayerSnapshot {
        return PlayerSnapshot(Arc<T>::make(rstd::move(value)));
    }
    auto clone() const -> PlayerSnapshot { return PlayerSnapshot(value_.clone()); }
    auto operator*() const -> const T& { return *value_; }
    auto operator->() const -> const T* { return &*value_; }
};
using PlayerStateSnapshot = PlayerSnapshot<p_qcm::p_msg::M_PlayerStateMsg>;
using PlayerQueueSnapshot = PlayerSnapshot<p_qcm::p_msg::M_PlayerQueueMsg>;

struct PlayerHost {
    template<class Self, class = void>
    struct Api {
        using Trait = PlayerHost;
        auto control(p_qcm::p_msg::M_PlayerControlReq request) const
            -> coro<PlayerIntegrationResult> {
            return rstd::trait_call<0>(this, rstd::move(request));
        }
        auto queue(p_qcm::p_msg::M_PlayerQueueReq request) const -> coro<PlayerIntegrationResult> {
            return rstd::trait_call<1>(this, rstd::move(request));
        }
        auto raise(String token = {}) const -> coro<PlayerIntegrationResult> {
            return rstd::trait_call<2>(this, rstd::move(token));
        }
        auto quit() const -> coro<PlayerIntegrationResult> { return rstd::trait_call<3>(this); }
    };
    template<class T>
    using Funcs = rstd::TraitFuncs<&T::control, &T::queue, &T::raise, &T::quit>;
};

struct PlayerIntegration {
    template<class Self, class = void>
    struct Api {
        using Trait = PlayerIntegration;
        auto update_state(PlayerStateSnapshot state) -> coro<PlayerIntegrationResult> {
            return rstd::trait_call<0>(this, rstd::move(state));
        }
        auto update_queue(PlayerQueueSnapshot queue) -> coro<PlayerIntegrationResult> {
            return rstd::trait_call<1>(this, rstd::move(queue));
        }
        auto shutdown() -> coro<empty> { return rstd::trait_call<2>(this); }
    };
    template<class T>
    using Funcs = rstd::TraitFuncs<&T::update_state, &T::update_queue, &T::shutdown>;
};

class PlayerIntegrations {
    Vec<Box<dyn<PlayerIntegration>>> children_;

public:
    void add(Box<dyn<PlayerIntegration>> child) { children_.push(rstd::move(child)); }
    auto is_empty() const -> bool { return children_.is_empty(); }
    auto update_state(PlayerStateSnapshot state) -> coro<PlayerIntegrationResult>;
    auto update_queue(PlayerQueueSnapshot queue) -> coro<PlayerIntegrationResult>;
    auto shutdown() -> coro<empty>;
};
} // namespace qcm::backend
