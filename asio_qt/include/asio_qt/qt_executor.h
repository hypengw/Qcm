#pragma once

#include <asio/execution.hpp>

#include "qt_execution_context.h"
#include "core/core.h"

class QtExecutor {
public:
    QtExecutor(rc<QtExecutionContext> ctx): m_ctx(ctx) {}

    QtExecutionContext& query(asio::execution::context_t) const noexcept { return *m_ctx; }

    static constexpr asio::execution::blocking_t query(asio::execution::blocking_t) noexcept {
        return asio::execution::blocking.never;
    }

    static constexpr asio::execution::relationship_t
    query(asio::execution::relationship_t) noexcept {
        return asio::execution::relationship.fork;
    }

    static constexpr asio::execution::outstanding_work_t
    query(asio::execution::outstanding_work_t) noexcept {
        return asio::execution::outstanding_work.tracked;
    }

    template<typename F>
    void execute(F f) const {
        m_ctx->post(std::move(f));
    }

    bool operator==(QtExecutor const& o) const noexcept { return m_ctx == o.m_ctx; }
    bool operator!=(QtExecutor const& o) const noexcept { return ! (*this == o); }

private:
    rc<QtExecutionContext> m_ctx;
};

static_assert(asio::execution::is_executor_v<QtExecutor>);
