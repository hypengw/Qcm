#pragma once

#include <deque>
#include <memory>
#include <algorithm>

#include "core/asio/basic.h"

namespace helper
{

template<typename Ex = asio::any_io_executor>
class AsyncLimit {
public:
    using executor_type = Ex;

    class block_guard {
    public:
        block_guard() = default;
        block_guard(AsyncLimit* lim, bool owns): m_lim(lim), m_owns(owns) {}
        block_guard(block_guard&& o) noexcept
            : m_lim(std::exchange(o.m_lim, nullptr)), m_owns(std::exchange(o.m_owns, false)) {}
        block_guard& operator=(block_guard&& o) noexcept {
            if (this != &o) {
                if (m_owns) release();
                m_lim  = std::exchange(o.m_lim, nullptr);
                m_owns = std::exchange(o.m_owns, false);
            }
            return *this;
        }
        ~block_guard() { release(); }
        void release() {
            if (m_owns && m_lim) {
                m_owns = false;
                m_lim->release_();
            }
        }
        block_guard(const block_guard&)            = delete;
        block_guard& operator=(const block_guard&) = delete;
        explicit     operator bool() const noexcept { return m_owns; }

    private:
        friend class AsyncLimit;
        AsyncLimit* m_lim  = nullptr;
        bool        m_owns = false;
    };

    explicit AsyncLimit(executor_type ex, std::size_t max_concurrency)
        : m_ex(ex),
          m_strand(asio::make_strand(ex)),
          m_max(max_concurrency),
          m_in_use(0),
          m_serial(0) {}

    executor_type get_executor() const noexcept { return m_ex; }

    void close() {
        asio::dispatch(m_strand, [this] {
            while (! m_waiters.empty()) {
                WrapperHandler w = std::move(m_waiters.front());
                m_waiters.pop_front();
                w.handler(asio::error::operation_aborted, block_guard {});
            }
        });
    }

    template<class CompletionToken>
    auto async_block(CompletionToken&& token) {
        using sig_t = void(asio::error_code, block_guard);
        return asio::async_initiate<CompletionToken, sig_t>(
            [this](auto handler) {
                auto h_exec  = asio::get_associated_executor(handler, m_ex);
                auto h_alloc = asio::get_associated_allocator(handler, std::allocator<void> {});
                auto slot    = asio::get_associated_cancellation_slot(handler);

                asio::dispatch(
                    m_strand, [this, h = std::move(handler), h_exec, h_alloc, slot]() mutable {
                        (void)h_alloc;
                        if (m_in_use < m_max) {
                            ++m_in_use;
                            asio::post(h_exec, [h = std::move(h), this]() mutable {
                                h(asio::error_code {}, block_guard { this, true });
                            });
                        } else {
                            auto wrapped_handle = WrapperHandler {
                                .serial = m_serial++,
                                .handler =
                                    [h = std::move(h), h_exec](asio::error_code c,
                                                               block_guard      g) mutable {
                                        asio::post(h_exec,
                                                   [h = std::move(h), c, g = std::move(g)] mutable {
                                                       h(c, std::move(g));
                                                   });
                                    }
                            };

                            if (slot.is_connected()) {
                                slot.assign([this, serial = wrapped_handle.serial](
                                                asio::cancellation_type) {
                                    asio::dispatch(m_strand, [this, serial] {
                                        auto it = std::find_if(m_waiters.begin(),
                                                               m_waiters.end(),
                                                               [serial](const auto& w) {
                                                                   return w.serial == serial;
                                                               });
                                        if (it != m_waiters.end()) {
                                            it->handler(asio::error::operation_aborted,
                                                        block_guard {});
                                            m_waiters.erase(it);
                                        }
                                    });
                                });
                            }

                            m_waiters.emplace_back(std::move(wrapped_handle));
                        }
                    });
            },
            std::forward<CompletionToken>(token));
    }

private:
    void release_() {
        asio::dispatch(m_strand, [this] {
            if (! m_waiters.empty()) {
                WrapperHandler w = std::move(m_waiters.front());
                m_waiters.pop_front();
                w.handler(asio::error_code {}, block_guard { this, true });
            } else {
                if (m_in_use > 0) --m_in_use;
            }
        });
    }

    executor_type               m_ex;
    asio::strand<executor_type> m_strand;
    const std::size_t           m_max;
    std::size_t                 m_in_use;

    struct WrapperHandler {
        std::uint32_t serial;

        asio::any_completion_handler<void(asio::error_code, block_guard)> handler;
    };

    std::uint32_t              m_serial;
    std::deque<WrapperHandler> m_waiters;
};
} // namespace helper