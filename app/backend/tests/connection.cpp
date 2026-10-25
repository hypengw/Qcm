#include <QCoreApplication>
#include <QHostAddress>
#include <QPointer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QWebSocket>
#include <QWebSocketServer>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

import qcm.backend.connection;
import rstd;

using namespace std::chrono_literals;

namespace
{
void require(bool condition, const char* message) {
    if (! condition) throw std::runtime_error(message);
}

struct Probe {
    std::mutex              mutex;
    std::condition_variable changed;
    int  connections {}, disconnections {}, held {}, handshakes {}, received {}, late {};
    bool wrong_thread {};
    std::vector<QByteArray> messages;
    std::vector<QString>    errors;

    template<class Fn>
    auto inspect(Fn fn) {
        auto lock = std::scoped_lock(mutex);
        return fn();
    }

    template<class Fn>
    void update(Fn fn) {
        auto lock = std::scoped_lock(mutex);
        fn();
        changed.notify_all();
    }

    template<class Fn>
    void wait(Fn predicate, const char* message) {
        auto lock = std::unique_lock(mutex);
        require(changed.wait_for(lock, 5s, predicate), message);
    }
};
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    auto             runtime = rstd::async::RuntimeBuilder::multi_thread()
                                   .worker_threads(rstd::usize(4))
                                   .enable_all()
                                   .build()
                                   .unwrap();
    Probe            probe;
    QWebSocketServer server(QStringLiteral("backend connection"), QWebSocketServer::NonSecureMode);
    require(server.listen(QHostAddress::LocalHost), "cannot listen");
    QTcpServer stalled;
    require(stalled.listen(QHostAddress::LocalHost), "cannot listen for stalled handshake");
    QPointer<QWebSocket> peer;
    QByteArray           held;
    int                  startup_messages {};
    QObject::connect(&server, &QWebSocketServer::newConnection, &app, [&] {
        auto socket = server.nextPendingConnection();
        peer        = socket;
        QObject::connect(socket, &QWebSocket::disconnected, socket, &QObject::deleteLater);
        QObject::connect(
            socket, &QWebSocket::binaryMessageReceived, socket, [&, socket](QByteArray bytes) {
                probe.update([&] {
                    ++probe.received;
                });
                if (bytes.startsWith("hold")) {
                    held = bytes;
                    probe.update([&] {
                        ++probe.held;
                    });
                } else {
                    socket->sendBinaryMessage(bytes);
                }
            });
        for (int i = 0; i < startup_messages; ++i)
            socket->sendBinaryMessage("startup-" + QByteArray::number(i));
    });
    QObject::connect(&stalled, &QTcpServer::newConnection, &app, [&] {
        while (stalled.hasPendingConnections()) {
            auto socket = stalled.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            probe.update([&] {
                ++probe.handshakes;
            });
        }
    });

    QThread owner;
    QObject bridge;
    bridge.moveToThread(&owner);
    owner.start();
    qcm::BackendConnection*                  connection {};
    qcm::BackendPending<QByteArray, QString> pending;
    bool                                     close_in_callback {};
    std::shared_future<void>                 callback_closed;
    const auto url         = QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort());
    const auto stalled_url = QStringLiteral("http://127.0.0.1:%1/").arg(stalled.serverPort());
    auto       on_owner    = [&](auto fn) {
        std::exception_ptr failure;
        require(QMetaObject::invokeMethod(
                    &bridge,
                    [&] {
                        try {
                            fn();
                        } catch (...) {
                            failure = std::current_exception();
                        }
                    },
                    Qt::BlockingQueuedConnection),
                "owner is unavailable");
        if (failure) std::rethrow_exception(failure);
    };
    auto on_peer = [&](auto fn) {
        require(QMetaObject::invokeMethod(&app, std::move(fn), Qt::BlockingQueuedConnection),
                "peer is unavailable");
    };
    auto check_thread = [&] {
        probe.wrong_thread |= QThread::currentThread() != &owner;
    };
    auto callbacks = [&] {
        qcm::BackendConnectionCallbacks result;
        result.connected = [&] {
            probe.update([&] {
                check_thread();
                ++probe.connections;
            });
        };
        result.message = [&](QByteArray bytes) {
            if (bytes.startsWith("sync-")) std::this_thread::sleep_for(1ms);
            if (bytes == "hold-in-flight") {
                bool delivered = pending.complete(1, std::move(bytes));
                probe.update([&] {
                    check_thread();
                    if (! delivered) ++probe.late;
                });
                return;
            }
            if (close_in_callback) {
                close_in_callback = false;
                callback_closed   = connection->shutdown();
            }
            probe.update([&] {
                check_thread();
                probe.messages.push_back(std::move(bytes));
            });
        };
        result.error = [&](QString error) {
            probe.update([&] {
                check_thread();
                probe.errors.push_back(std::move(error));
            });
        };
        result.disconnected = [&] {
            probe.update([&] {
                check_thread();
                ++probe.disconnections;
            });
        };
        return result;
    };
    qcm::BackendConnectionLimits limits;
    limits.frame_bytes   = 2 * 1024 * 1024;
    limits.message_bytes = 4 * 1024 * 1024;
    limits.receive_bytes = 8 * 1024 * 1024;
    limits.send_messages = 8;
    limits.send_bytes    = 3 * 1024 * 1024;
    int         result   = 0;
    std::thread driver([&] {
        auto close = [&] {
            if (! connection) return;
            std::shared_future<void> closed;
            on_owner([&] {
                closed = connection->shutdown();
            });
            require(closed.wait_for(6s) == std::future_status::ready, "shutdown timed out");
            closed.get();
            on_owner([&] {
                delete connection;
                connection = nullptr;
            });
        };
        auto create = [&](qcm::BackendConnectionLimits configured) {
            auto count  = probe.inspect([&] {
                return probe.connections;
            });
            auto errors = probe.inspect([&] {
                return probe.errors.size();
            });
            on_owner([&] {
                connection = new qcm::BackendConnection(runtime.handle(), callbacks(), configured);
                connection->open(url);
            });
            probe.wait(
                [&] {
                    return probe.connections > count || probe.errors.size() > errors;
                },
                "connect timed out");
            probe.inspect([&] {
                require(probe.connections > count, "connect failed");
            });
        };
        try {
            on_peer([&] {
                startup_messages = 512;
            });
            bool startup_send {}, startup_before_connected {};
            on_owner([&] {
                auto events      = callbacks();
                auto connected   = std::move(events.connected);
                auto message     = std::move(events.message);
                events.connected = [&, connected = std::move(connected)] {
                    connected();
                    startup_send = connection->send("from-connected").is_ok();
                };
                events.message = [&, message = std::move(message)](QByteArray bytes) {
                    probe.update([&] {
                        startup_before_connected |= probe.connections == 0;
                    });
                    message(std::move(bytes));
                };
                connection =
                    new qcm::BackendConnection(runtime.handle(), std::move(events), limits);
                connection->open(url);
            });
            probe.wait(
                [&] {
                    return probe.messages.size() >= 513 || ! probe.errors.empty();
                },
                "startup messages timed out");
            on_owner([&] {
                require(startup_send, "connected callback could not send");
            });
            probe.inspect([&] {
                require(! startup_before_connected, "message preceded connected");
                require(probe.errors.empty(), "startup burst disconnected");
                require(probe.messages.size() == 513, "startup messages were lost");
                for (int i = 0; i < 512; ++i)
                    require(probe.messages[i] == "startup-" + QByteArray::number(i),
                            "startup message order failed");
                require(probe.messages.back() == "from-connected", "startup reply was reordered");
            });
            close();
            on_peer([&] {
                startup_messages = 2;
            });
            auto messages    = probe.inspect([&] {
                return probe.messages.size();
            });
            auto connections = probe.inspect([&] {
                return probe.connections;
            });
            on_owner([&] {
                auto events      = callbacks();
                auto connected   = std::move(events.connected);
                events.connected = [&, connected = std::move(connected)] {
                    connected();
                    callback_closed = connection->shutdown();
                };
                connection =
                    new qcm::BackendConnection(runtime.handle(), std::move(events), limits);
                connection->open(url);
            });
            probe.wait(
                [&] {
                    return probe.connections > connections;
                },
                "startup close did not connect");
            std::shared_future<void> startup_closed;
            on_owner([&] {
                startup_closed = callback_closed;
            });
            require(startup_closed.wait_for(6s) == std::future_status::ready,
                    "shutdown in connected callback deadlocked");
            startup_closed.get();
            probe.inspect([&] {
                require(probe.messages.size() == messages, "startup close delivered messages");
            });
            close();

            connections             = probe.inspect([&] {
                return probe.connections;
            });
            bool replace_on_message = true;
            on_owner([&] {
                auto events    = callbacks();
                auto message   = std::move(events.message);
                events.message = [&, message = std::move(message)](QByteArray bytes) {
                    message(std::move(bytes));
                    if (std::exchange(replace_on_message, false)) connection->open(url);
                };
                connection =
                    new qcm::BackendConnection(runtime.handle(), std::move(events), limits);
                connection->open(url);
            });
            probe.wait(
                [&] {
                    return probe.connections == connections + 2 &&
                           probe.messages.size() >= messages + 3;
                },
                "startup replacement timed out");
            probe.inspect([&] {
                require(probe.messages.size() == messages + 3,
                        "replacement delivered stale startup messages");
                require(probe.messages[messages] == "startup-0" &&
                            probe.messages[messages + 1] == "startup-0" &&
                            probe.messages[messages + 2] == "startup-1",
                        "replacement startup order failed");
                require(probe.errors.empty(), "startup lifecycle published a failure");
            });
            close();
            on_peer([&] {
                startup_messages = 0;
            });
            probe.update([&] {
                probe.connections = probe.disconnections = probe.received = 0;
                probe.messages.clear();
            });
            std::puts("immediate startup push, connected send, shutdown and replacement passed");

            create(limits);
            bool count_limit {}, oversized {};
            on_owner([&] {
                for (int i = 0; i < 8; ++i)
                    require(connection->send(QByteArray::number(i)).is_ok(), "burst rejected");
                count_limit = connection->send("overflow").is_err();
                oversized   = connection->send(QByteArray(2 * 1024 * 1024 + 1, 'x')).is_err();
            });
            require(count_limit && oversized, "queue or message limit failed");
            probe.wait(
                [&] {
                    return probe.messages.size() >= 8;
                },
                "burst responses timed out");
            probe.inspect([&] {
                const std::vector<QByteArray> expected { "0", "1", "2", "3", "4", "5", "6", "7" };
                require(probe.messages == expected, "send order failed");
                require(probe.errors.empty(), "burst disconnected");
            });

            QByteArray large(1536 * 1024, 'a');
            bool       byte_limit {};
            on_owner([&] {
                require(connection->send(large).is_ok(), "large message rejected");
                byte_limit = connection->send(QByteArray(2 * 1024 * 1024, 'b')).is_err();
            });
            require(byte_limit, "queue byte limit failed");
            probe.wait(
                [&] {
                    return probe.messages.size() >= 9;
                },
                "large response timed out");
            probe.inspect([&] {
                require(probe.messages[8] == large, "large response corrupted");
            });

            auto request = rstd::async::Completion<QByteArray, QString>::make().unwrap();
            on_owner([&] {
                require(pending.add(1, rstd::move(request.get<1>())),
                        "request registration failed");
                require(connection->send("hold-in-flight").is_ok(), "hold rejected");
                pending.cancel(1);
            });
            auto canceled = [&]() -> rstd::async::coro<void> {
                auto response = co_await rstd::move(request.get<0>());
                require(response.is_err() && response.unwrap_err().is_canceled(),
                        "business cancellation failed");
            };
            runtime.block_on(canceled());
            probe.wait(
                [&] {
                    return probe.held == 1;
                },
                "cancellation discarded an accepted send");
            on_peer([&] {
                peer->sendBinaryMessage(held);
            });
            probe.wait(
                [&] {
                    return probe.late == 1;
                },
                "late response was not discarded");
            on_owner([&] {
                require(connection->send("after-cancel").is_ok(), "cancel damaged send");
            });
            probe.wait(
                [&] {
                    return probe.messages.size() >= 10;
                },
                "cancel damaged connection");

            on_peer([&] {
                peer->abort();
            });
            probe.wait(
                [&] {
                    return probe.disconnections >= 1;
                },
                "peer exit was not reported");
            on_owner([&] {
                connection->open(url);
            });
            probe.wait(
                [&] {
                    return probe.connections == 2;
                },
                "retry timed out");
            on_owner([&] {
                require(connection->send("retry").is_ok(), "retry send failed");
            });
            probe.wait(
                [&] {
                    return probe.messages.size() >= 11;
                },
                "retry response timed out");

            on_owner([&] {
                connection->open(stalled_url);
            });
            probe.wait(
                [&] {
                    return probe.handshakes >= 1;
                },
                "handshake did not start");
            on_owner([&] {
                connection->open(url);
            });
            probe.wait(
                [&] {
                    return probe.connections == 3;
                },
                "replacement connect timed out");
            auto errors = probe.inspect([&] {
                return probe.errors.size();
            });
            on_owner([&] {
                require(connection->send("generation").is_ok(), "generation send failed");
            });
            probe.wait(
                [&] {
                    return probe.messages.size() >= 12;
                },
                "replacement response timed out");
            probe.inspect([&] {
                require(probe.messages[11] == "generation", "old generation changed payload");
                require(probe.errors.size() == errors, "old generation published a failure");
            });
            std::puts("queue bounds, FIFO, business cancellation, large messages and retry passed");

            messages = probe.inspect([&] {
                return probe.messages.size();
            });
            on_peer([&] {
                for (int i = 0; i < 512; ++i)
                    peer->sendBinaryMessage("sync-" + QByteArray::number(i));
            });
            probe.wait(
                [&] {
                    return probe.messages.size() >= messages + 512 || probe.errors.size() > errors;
                },
                "sync burst responses timed out");
            probe.inspect([&] {
                require(probe.errors.size() == errors, "sync burst disconnected");
                for (int i = 0; i < 512; ++i)
                    require(probe.messages[messages + i] == "sync-" + QByteArray::number(i),
                            "sync burst lost message order");
            });
            std::puts("sync burst larger than the receive queue was delivered in order");

            std::promise<void> idle;
            auto               elapsed = idle.get_future();
            on_peer([&] {
                QTimer::singleShot(31000, &app, [&] {
                    idle.set_value();
                });
            });
            require(elapsed.wait_for(33s) == std::future_status::ready, "idle timer did not run");
            on_owner([&] {
                require(connection->send("after-idle").is_ok(), "idle closed IPC");
            });
            probe.wait(
                [&] {
                    return probe.messages.size() >= messages + 513;
                },
                "idle response timed out");
            probe.inspect([&] {
                require(probe.messages.back() == "after-idle", "idle response corrupted");
                require(probe.errors.size() == errors, "idle published a failure");
            });
            std::puts("IPC survived the HTTP default total timeout");

            on_owner([&] {
                close_in_callback = true;
                require(connection->send("shutdown-in-callback").is_ok(), "callback send rejected");
            });
            probe.wait(
                [&] {
                    return probe.messages.size() >= messages + 514;
                },
                "callback shutdown did not start");
            std::shared_future<void> closed;
            on_owner([&] {
                closed = callback_closed;
            });
            require(closed.wait_for(6s) == std::future_status::ready,
                    "callback shutdown deadlocked");
            closed.get();
            close();

            on_owner([&] {
                connection = new qcm::BackendConnection(runtime.handle(), callbacks(), limits);
                connection->open(stalled_url);
            });
            probe.wait(
                [&] {
                    return probe.handshakes >= 2;
                },
                "shutdown handshake did not start");
            close();

            create(limits);
            auto received = probe.inspect([&] {
                return probe.received;
            });
            on_owner([&] {
                for (int i = 0; i < 8; ++i)
                    require(connection->send("drain-" + QByteArray::number(i)).is_ok(),
                            "drain rejected");
                (void)connection->shutdown();
                require(connection->send("too-late").is_err(), "shutdown accepted a new send");
            });
            close();
            probe.wait(
                [&] {
                    return probe.received >= received + 8;
                },
                "shutdown lost accepted sends");
            std::puts("callback, connecting and queued-send shutdown drained the Qt owner");

            errors = probe.inspect([&] {
                return probe.errors.size();
            });
            on_owner([&] {
                auto limited         = limits;
                limited.engine_bytes = 1;
                connection = new qcm::BackendConnection(runtime.handle(), callbacks(), limited);
                connection->open(url);
            });
            probe.wait(
                [&] {
                    return probe.errors.size() > errors;
                },
                "engine capacity failure was hidden");
            on_owner([&] {
                require(connection->send("no-budget").is_err(), "failed connect accepted send");
            });
            close();

            create({});
            QByteArray response(20 * 1024 * 1024, 'z');
            messages = probe.inspect([&] {
                return probe.messages.size();
            });
            on_peer([&] {
                peer->setOutgoingFrameSize(response.size());
                peer->sendBinaryMessage(response);
            });
            probe.wait(
                [&] {
                    return probe.messages.size() > messages;
                },
                "single-frame large response failed");
            probe.inspect([&] {
                require(probe.messages.back() == response, "large frame corrupted");
            });
            std::puts("default receive budget accepted a 20 MiB single-frame response");
            errors = probe.inspect([&] {
                return probe.errors.size();
            });
            on_peer([&] {
                peer->sendTextMessage(QStringLiteral("invalid backend payload"));
            });
            probe.wait(
                [&] {
                    return probe.errors.size() > errors;
                },
                "non-binary input was accepted");
            close();
            probe.inspect([&] {
                require(! probe.wrong_thread, "callback used wrong thread");
            });
            std::puts("capacity and non-binary failures were reported on the Qt owner");
        } catch (const std::exception& error) {
            std::fprintf(stderr, "backend connection: %s\n", error.what());
            result = 1;
        }
        try {
            close();
            on_owner([&] {
                bridge.moveToThread(app.thread());
            });
        } catch (const std::exception& error) {
            std::fprintf(stderr, "backend cleanup: %s\n", error.what());
            std::abort();
        }
        owner.quit();
        owner.wait();
        QMetaObject::invokeMethod(&app, &QCoreApplication::quit, Qt::QueuedConnection);
    });
    QTimer::singleShot(75000, &app, [] {
        std::fputs("backend connection test timed out\n", stderr);
        std::abort();
    });
    app.exec();
    driver.join();
    return result;
}
