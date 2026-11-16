export module qcm.backend:lua_memory;
import luato;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
rstd::sync::atomic::Atomic<u64> next_lua_memory_id { u64(1) };

class LuaMemorySummary {
    Option<luato::MemoryObserver> memory_;
    String                        kind_;
    i64                           provider_ {};
    u64                           id_ {};

    void report(ref<str> event) const {
        if (memory_.is_none()) return;
        constexpr auto target = rstd::log::Target { "qcm.lua_allocator"_str };
        auto           stats  = memory_->snapshot();
        rstd::log::debug(target,
                         "lua vm event={} vm={} kind={} provider={} closed={} used_bytes={} "
                         "peak_used_bytes={} live_allocations={} allocations={} deallocations={} "
                         "reallocations={} failures={} limit_failures={} control_bytes={}",
                         event,
                         id_,
                         kind_,
                         provider_,
                         stats.closed,
                         stats.used_bytes,
                         stats.peak_used_bytes,
                         stats.live_allocations,
                         stats.allocations,
                         stats.deallocations,
                         stats.reallocations,
                         stats.failures,
                         stats.limit_failures,
                         stats.control_bytes);
        if (stats.pool.is_none()) return;
        const auto& pool   = *stats.pool;
        const auto& memory = *stats.upstream;
        rstd::log::debug(
            target,
            "lua pool event={} vm={} kind={} provider={} live_bytes={} live_block_bytes={} "
            "reserved_bytes={} retained_bytes={} metadata_bytes={} slabs={} reuses={} "
            "mapped_bytes={} mapped_bytes_peak={} allocation_calls={} "
            "deallocation_calls={}",
            event,
            id_,
            kind_,
            provider_,
            pool.live_bytes,
            pool.live_block_bytes,
            pool.reserved_bytes,
            pool.retained_bytes,
            pool.metadata_bytes,
            pool.slabs,
            pool.reuses,
            memory.mapped_bytes,
            memory.mapped_bytes_peak,
            memory.allocation_calls,
            memory.deallocation_calls);
    }

public:
    LuaMemorySummary(const luato::State& state, ref<str> kind, Option<i64> provider = None()) {
        if (! rstd::log::log_enabled(rstd::log::Level::Debug, "qcm.lua_allocator"_str)) return;
        auto observer = state.memory_observer();
        if (observer.is_err()) return;
        memory_   = Some(rstd::move(observer).unwrap());
        kind_     = String::make(kind);
        provider_ = provider.unwrap_or(i64());
        id_       = next_lua_memory_id.fetch_add(u64(1), rstd::sync::atomic::Ordering::Relaxed);
        report("created"_str);
    }
    LuaMemorySummary(LuaMemorySummary&& other) noexcept
        : memory_(other.memory_.take()),
          kind_(rstd::move(other.kind_)),
          provider_(other.provider_),
          id_(other.id_) {}
    auto operator=(LuaMemorySummary&& other) noexcept -> LuaMemorySummary& {
        if (this != &other) {
            report("replaced"_str);
            memory_   = other.memory_.take();
            kind_     = rstd::move(other.kind_);
            provider_ = other.provider_;
            id_       = other.id_;
        }
        return *this;
    }
    ~LuaMemorySummary() { report("released"_str); }
};
} // namespace qcm::backend
