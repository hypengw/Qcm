#include <QByteArray>
#include <QString>
#include <cstdio>
#include <exception>
#include <stdexcept>

import qcm.backend.connection;
import rstd;

namespace
{
void require(bool condition, const char* message) {
    if (! condition) throw std::runtime_error(message);
}
} // namespace

int main() {
    try {
        auto runtime = rstd::async::RuntimeBuilder::current_thread().enable_all().build().unwrap();
        std::exception_ptr failure;
        auto               run = [&]() -> rstd::async::coro<void> {
            try {
                using Completion = rstd::async::Completion<QByteArray, QString>;
                qcm::BackendPending<QByteArray, QString> pending;
                auto                                     first  = Completion::make().unwrap();
                auto                                     second = Completion::make().unwrap();
                require(pending.add(1, rstd::move(first.get<1>())), "first request rejected");
                require(pending.add(2, rstd::move(second.get<1>())), "second request rejected");
                require(pending.contains(1), "request was not registered");
                pending.cancel(1);
                auto canceled = co_await rstd::move(first.get<0>());
                require(canceled.is_err() && canceled.unwrap_err().is_canceled(),
                        "business cancellation did not release its response");
                require(! pending.complete(1, "late"), "canceled response was delivered");
                require(pending.complete(2, "second"), "other request was canceled");
                require(! pending.complete(2, "duplicate"), "response completed twice");
                auto received = co_await rstd::move(second.get<0>());
                require(received.is_ok() && received.unwrap() == "second", "wrong response");

                auto dropped = Completion::make().unwrap();
                require(pending.add(3, rstd::move(dropped.get<1>())), "dropped request rejected");
                dropped.get<0>().close();
                require(pending.complete(3, "already canceled"), "closed receiver was not removed");
                require(! pending.contains(3), "closed receiver stayed registered");

                auto failed    = Completion::make().unwrap();
                auto duplicate = Completion::make().unwrap();
                require(pending.add(4, rstd::move(failed.get<1>())), "failed request rejected");
                require(! pending.add(4, rstd::move(duplicate.get<1>())), "duplicate id accepted");
                auto rejected = co_await rstd::move(duplicate.get<0>());
                require(rejected.is_err() && rejected.unwrap_err().is_canceled(),
                        "duplicate replaced the original response");
                pending.fail_all(QStringLiteral("disconnected"));
                auto error = co_await rstd::move(failed.get<0>());
                require(error.is_err(), "pending was not failed");
                auto reason = rstd::move(error).unwrap_err();
                require(reason.is_failed(), "pending failure was reported as cancellation");
                require(rstd::move(reason).unwrap_failed() == QStringLiteral("disconnected"),
                        "disconnect reason was lost");
                require(! pending.contains(4) && ! pending.complete(4, "late"),
                        "failed response stayed registered");
            } catch (...) {
                failure = std::current_exception();
            }
        };
        runtime.block_on(run());
        if (failure) std::rethrow_exception(failure);
        std::puts("pending cancellation, late responses and one-shot completion passed");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "backend pending: %s\n", error.what());
        return 1;
    }
}
