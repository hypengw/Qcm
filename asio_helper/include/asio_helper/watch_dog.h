#pragma once

#include <asio/steady_timer.hpp>
#include <asio/awaitable.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/deferred.hpp>
#include <asio/co_spawn.hpp>
#include <asio/as_tuple.hpp>

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

    template<typename Ex, typename F>
    auto watch(Ex&& ex, F&& f, const duration& t = asio::chrono::minutes(5)) {
        cancel();
        m_timer = std::make_shared<asio::steady_timer>(ex);
        m_timer->expires_after(t);
        return ([](rc<asio::steady_timer> timer, std::decay_t<F> f) -> asio::awaitable<void> {
            auto ex = co_await asio::this_coro::executor;
            auto [order, exp, ec] =
                co_await asio::experimental::make_parallel_group(
                    asio::co_spawn(ex, std::move(f), asio::deferred),
                    asio::co_spawn(ex, timer->async_wait(asio::use_awaitable), asio::deferred))
                    .async_wait(asio::experimental::wait_for_one(), asio::deferred);

            timer->expires_after(asio::chrono::seconds(0));

            if (exp) std::rethrow_exception(exp);
            // if (exp2) std::rethrow_exception(exp2);
            co_return;
        })(m_timer, std::move(f));
    }

    void cancel() {
        if (m_timer) {
            m_timer->cancel();
            m_timer->expires_after(asio::chrono::seconds(0));
        }
    }

private:
    rc<asio::steady_timer> m_timer;
};
} // namespace helper
