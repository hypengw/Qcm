export module qcm.asio:watch_dog;
export import :task;
export import qcm.core;
export import asio;

namespace qcm
{

export class WatchDog {
public:
    WatchDog() {}
    ~WatchDog() { cancel(); }

    using clock    = asio::steady_timer::clock_type;
    using duration = asio::steady_timer::duration;

    bool is_running() const {
        if (m_timer) {
            return cppstd::chrono::operator>(m_timer->expiry(), clock::now());
        }
        return false;
    }

    template<typename Ex, typename F, typename Allocator = cppstd::allocator<void>>
        requires(! rstd::mtp::same_as<duration, rstd::mtp::remove_cvref_t<Allocator>>)
    auto watch(Ex&& ex, F&& f, const duration& t = asio::chrono::minutes(5), Allocator&& alloc = {})
        -> asio::awaitable<void> {
        cancel();
        m_timer = std::make_shared<asio::steady_timer>(ex);
        m_timer->expires_after(t);
        return watch_impl<rstd::mtp::decay_t<F>>(m_timer, rstd::move(f), alloc);
    }

    void cancel() {
        if (m_timer) {
            m_timer->cancel();
            m_timer->expires_after(asio::chrono::seconds(0));
        }
    }

    template<typename Ex, typename F, typename CT,
             typename Allocator = cppstd::allocator<void>>
    auto spawn(Ex&& ex, F&& f, CT&& ct, const duration& t = asio::chrono::minutes(5),
               Allocator&& alloc = {}) {
        asio::co_spawn(ex, watch(ex, rstd::forward<F>(f), t, alloc), rstd::forward<CT>(ct));
    }

private:
    template<typename F, typename Allocator>
    static auto watch_impl(rc<asio::steady_timer> timer, F f, Allocator alloc)
        -> asio::awaitable<void> {
        auto ex = co_await asio::this_coro::executor_;
        auto [order, exp, ec] =
            co_await asio::experimental::make_parallel_group(
                asio::co_spawn(ex, rstd::move(f), asio::bind_allocator(alloc, deferred)),
                asio::co_spawn(
                    ex, timer->async_wait(use_task), asio::bind_allocator(alloc, deferred)))
                .async_wait(asio::experimental::wait_for_one(), deferred);

        timer->expires_after(asio::chrono::seconds(0));

        if (exp) cppstd::rethrow_exception(exp);
        co_return;
    }

    rc<asio::steady_timer> m_timer;
};
} // namespace qcm
