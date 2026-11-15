export module qcm.backend:player.integration;
export import :protocol;
export import rstd;

using rstd::async::coro;
using namespace rstd::prelude;

export namespace qcm::backend
{
using PlayerIntegrationResult = Result<empty, String>;

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
        auto raise() const -> coro<PlayerIntegrationResult> { return rstd::trait_call<2>(this); }
        auto quit() const -> coro<PlayerIntegrationResult> { return rstd::trait_call<3>(this); }
    };
    template<class T>
    using Funcs = rstd::TraitFuncs<&T::control, &T::queue, &T::raise, &T::quit>;
};

struct PlayerIntegration {
    template<class Self, class = void>
    struct Api {
        using Trait = PlayerIntegration;
        auto update_state(p_qcm::p_msg::M_PlayerStateMsg state) -> coro<PlayerIntegrationResult> {
            return rstd::trait_call<0>(this, rstd::move(state));
        }
        auto update_queue(p_qcm::p_msg::M_PlayerQueueMsg queue) -> coro<PlayerIntegrationResult> {
            return rstd::trait_call<1>(this, rstd::move(queue));
        }
        auto shutdown() -> coro<empty> { return rstd::trait_call<2>(this); }
    };
    template<class T>
    using Funcs = rstd::TraitFuncs<&T::update_state, &T::update_queue, &T::shutdown>;
};
} // namespace qcm::backend
