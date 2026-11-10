export module qcm.backend:provider.generation;
export import rstd;
using rstd::sync::Arc;
using rstd::sync::atomic::Atomic;
using rstd::sync::atomic::Ordering;
using namespace rstd::prelude;

namespace qcm::backend
{
struct ProviderGenerationState {
    Atomic<u64>  epoch { u64(1) };
    Atomic<bool> active { true };
};
} // namespace qcm::backend

export namespace qcm::backend
{
class ProviderGeneration;
class ProviderGenerationCheck {
    Arc<ProviderGenerationState> state_;
    u64                          expected_;

    ProviderGenerationCheck(Arc<ProviderGenerationState> state, u64 expected)
        : state_(rstd::move(state)), expected_(expected) {}
    friend class ProviderGeneration;

public:
    auto clone() const -> ProviderGenerationCheck {
        return ProviderGenerationCheck(state_.clone(), expected_);
    }
    auto is_current() const -> bool {
        return state_->active.load(Ordering::Acquire) &&
               state_->epoch.load(Ordering::Acquire) == expected_;
    }
    auto same_generation(const ProviderGenerationCheck& other) const -> bool {
        return Arc<ProviderGenerationState>::ptr_eq(state_, other.state_) &&
               expected_ == other.expected_;
    }
};

// Only the lifecycle owner publishes changes; checks may be held by DB workers.
class ProviderGeneration {
    Arc<ProviderGenerationState> state_ { Arc<ProviderGenerationState>::make() };

public:
    ProviderGeneration()                                             = default;
    ProviderGeneration(const ProviderGeneration&)                    = delete;
    auto operator=(const ProviderGeneration&) -> ProviderGeneration& = delete;
    ProviderGeneration(ProviderGeneration&&) noexcept                = default;
    auto operator=(ProviderGeneration&&) -> ProviderGeneration&      = delete;
    ~ProviderGeneration() {
        if (state_) state_->active.store(false, Ordering::Release);
    }
    auto current() const -> u64 { return state_->epoch.load(Ordering::Acquire); }
    auto snapshot() const -> ProviderGenerationCheck {
        return ProviderGenerationCheck(state_.clone(), current());
    }
    auto advance() -> bool {
        if (current() == u64::MAX) {
            state_->active.store(false, Ordering::Release);
            return false;
        }
        (void)state_->epoch.fetch_add(u64(1), Ordering::AcqRel);
        return true;
    }
    auto invalidate() -> void {
        (void)advance();
        state_->active.store(false, Ordering::Release);
    }
};
} // namespace qcm::backend
