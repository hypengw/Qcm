export module qcm.backend:player.integration;
export import :protocol;
export import rstd;

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
    auto update_state(PlayerStateSnapshot state) -> coro<PlayerIntegrationResult> {
        Option<String> error;
        for (auto& child : children_) {
            auto result = co_await child->update_state(state.clone());
            if (result.is_err() && error.is_none()) error = Some(rstd::move(result).unwrap_err());
        }
        if (error.is_some()) co_return Err(rstd::move(*error));
        co_return Ok(empty {});
    }
    auto update_queue(PlayerQueueSnapshot queue) -> coro<PlayerIntegrationResult> {
        Option<String> error;
        for (auto& child : children_) {
            auto result = co_await child->update_queue(queue.clone());
            if (result.is_err() && error.is_none()) error = Some(rstd::move(result).unwrap_err());
        }
        if (error.is_some()) co_return Err(rstd::move(*error));
        co_return Ok(empty {});
    }
    auto shutdown() -> coro<empty> {
        while (! children_.is_empty()) {
            auto child = children_.pop().unwrap();
            co_await child->shutdown();
        }
        co_return empty {};
    }
};
} // namespace qcm::backend
