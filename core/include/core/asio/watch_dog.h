#pragma once

#include <asio/steady_timer.hpp>
#include <asio/awaitable.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/deferred.hpp>
#include <asio/co_spawn.hpp>
#include <asio/as_tuple.hpp>
#include <asio/bind_allocator.hpp>

#include "core/core.h"
#include "core/log.h"

namespace helper
{

class WatchDog {
public:
    WatchDog() {}
    ~WatchDog() { cancel(); }

    using clock    = asio::steady_timer::clock_type;
    using duration = asio::steady_timer::duration;

    bool is_running() const {
        if (m_timer) {
            return m_timer->expiry() > clock::now();
        }
        return false;
    }

    template<typename Ex, typename F, typename Allocator = std::allocator<void>>
        requires(! std::same_as<duration, std::remove_cvref_t<Allocator>>)
    auto watch(Ex&& ex, F&& f, const duration& t = asio::chrono::minutes(5),
               Allocator&& alloc = {}) -> asio::awaitable<void> {
        cancel();
        m_timer = std::make_shared<asio::steady_timer>(ex);
        m_timer->expires_after(t);
        return watch_impl<std::decay_t<F>>(m_timer, std::move(f), alloc);
    }

    void cancel() {
        if (m_timer) {
            m_timer->cancel();
            m_timer->expires_after(asio::chrono::seconds(0));
        }
    }

    template<typename Ex, typename F, typename CT, typename Allocator = std::allocator<void>>
    auto spawn(Ex&& ex, F&& f, CT&& ct, const duration& t = asio::chrono::minutes(5),
               Allocator&& alloc = {}) {
        asio::co_spawn(ex, watch(ex, std::forward<F>(f), t, alloc), std::forward<CT>(ct));
    }

private:
    template<typename F, typename Allocator>
    static auto watch_impl(rc<asio::steady_timer> timer, F f,
                           Allocator alloc) -> asio::awaitable<void> {
        auto ex = co_await asio::this_coro::executor;
        auto [order, exp, ec] =
            co_await asio::experimental::make_parallel_group(
                asio::co_spawn(ex, std::move(f), asio::bind_allocator(alloc, asio::deferred)),
                asio::co_spawn(ex,
                               timer->async_wait(asio::use_awaitable),
                               asio::bind_allocator(alloc, asio::deferred)))
                .async_wait(asio::experimental::wait_for_one(), asio::deferred);

        timer->expires_after(asio::chrono::seconds(0));

        if (exp) std::rethrow_exception(exp);
        co_return;
    }

    rc<asio::steady_timer> m_timer;
};
} // namespace helper
