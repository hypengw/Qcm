module;
#include <cstdio>
#include <exception>
#include <future>
#include <limits>
#include <memory>
#include <rstd/macro.hpp>

#include "Qcm/backend.moc.h"
#include "core/log.h"

module qcm;
import :backend;
import :status.process;
import qcm.backend.connection;
import platform;
import qcm.log;
import qextra;
import rstd;

using namespace qcm;
using namespace Qt::Literals::StringLiterals;
using namespace qextra::prelude;

namespace qcm
{

namespace detail
{

using ResponseHandle = rstd::async::CompletionHandle<msg::QcmMessage, msg::Error>;

auto transport_error(QString message) -> msg::Error {
    return msg::Error { .code = -1, .message = message.toStdString() };
}

class BackendTransport : public QObject {
public:
    explicit BackendTransport(Backend* backend): m_backend(backend) {}

    void initialize() {
        plt::set_thread_name("backend");

        m_process = std::make_unique<QProcess>();
        m_process->setWorkingDirectory(QCoreApplication::applicationDirPath());
        m_process->setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedErrorChannel);

        connect(m_process.get(),
                &QProcess::finished,
                this,
                [this](int exit_code, QProcess::ExitStatus) {
                    auto message = QStringLiteral("Backend exited: %1").arg(exit_code);
                    fail(message);
                });
        connect(
            m_process.get(), &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
                auto key     = QMetaEnum::fromType<QProcess::ProcessError>().valueToKey(error);
                auto message = QStringLiteral("Backend %1").arg(QString::fromLatin1(key));
                fail(message);
            });

        m_serializer = std::make_unique<QProtobufSerializer>();
        BackendConnectionCallbacks callbacks;
        callbacks.connected = [this] {
            m_connected = true;
            Q_EMIT m_backend->connected(m_port);
        };
        callbacks.error = [this](QString message) {
            fail(message);
        };
        callbacks.disconnected = [this] {
            m_connected = false;
            fail_pending(QStringLiteral("backend disconnected"));
            Q_EMIT m_backend->disconnected();
        };
        callbacks.message = [this](QByteArray bytes) {
            msg::QcmMessage message;
            if (! message.deserialize(m_serializer.get(), bytes)) {
                fail(QStringLiteral("Invalid backend protobuf message"));
                return;
            }

            if (message.type() != msg::MessageTypeGadget::MessageType::PROVIDER_SYNC_STATUS_MSG) {
                LOG_INFO("ws recv: {}", message.type());
            }

            auto id = message.id_proto();
            if (id != 0) {
                (void)m_pending.complete(id, rstd::move(message));
                return;
            }

            (void)QMetaObject::invokeMethod(
                m_backend,
                [message = rstd::move(message)]() mutable {
                    qcm::process_msg(rstd::move(message));
                },
                Qt::QueuedConnection);
        };
        m_connection = std::make_unique<BackendConnection>(QAsyncResult::runtime_handle(),
                                                           std::move(callbacks));
    }

    void start(QString exe, QString data_dir, QString cache_dir) {
        if (! m_process || m_process->state() != QProcess::NotRunning) return;
        if (m_shutting_down) return;
        m_failed    = false;
        m_connected = false;
        m_connection->disconnect();

        if (m_stdout_connection) QObject::disconnect(m_stdout_connection);
        m_stdout_connection =
            connect(m_process.get(), &QProcess::readyReadStandardOutput, this, [this] {
                m_process->setReadChannel(QProcess::StandardOutput);
                if (! m_process->canReadLine()) return;

                auto document = QJsonDocument::fromJson(m_process->readLine());
                auto port     = document.object().value("port");
                if (port.isUndefined()) {
                    Q_EMIT m_backend->error(QStringLiteral("Read port from backend failed"));
                    return;
                }

                auto value = port.toVariant().value<i32>();
                LOG_INFO("backend port: {}", value);
                Q_EMIT m_backend->started(value);

                QObject::disconnect(m_stdout_connection);
                m_stdout_connection =
                    connect(m_process.get(), &QProcess::readyReadStandardOutput, this, [this] {
                        auto output = m_process->readAllStandardOutput();
                        if (! output.isEmpty()) {
                            std::fwrite(output.constData(),
                                        1,
                                        static_cast<std::size_t>(output.size()),
                                        stdout);
                            std::fflush(stdout);
                        }
                    });
            });

        LOG_DEBUG("starting backend: {}", exe.toStdString());
        m_process->start(exe, { u"--data"_s, data_dir, u"--cache"_s, cache_dir });
    }

    void kill() {
        m_connected = false;
        if (m_connection) m_connection->disconnect();
        if (m_process && m_process->state() != QProcess::NotRunning) m_process->kill();
    }

    void connect_to(rstd::string::String url, i32 port) {
        m_port = port;
        if (m_connection && ! m_shutting_down) m_connection->open(qextra::to_qstring(url));
    }

    void send(msg::QcmMessage message, ResponseHandle response) {
        if (! m_connection || ! m_connected || m_shutting_down) {
            (void)response.fail(transport_error(QStringLiteral("backend disconnected")));
            return;
        }

        auto id = message.id_proto();
        if (m_pending.contains(id)) {
            (void)response.fail(transport_error(QStringLiteral("duplicate backend request id")));
            return;
        }
        auto sent = m_connection->send(message.serialize(m_serializer.get()));
        if (sent.is_err()) {
            (void)response.fail(transport_error(sent.unwrap_err()));
            return;
        }
        (void)m_pending.add(id, rstd::move(response));
    }

    void send_untracked(msg::QcmMessage message) {
        if (! m_connection || m_shutting_down) return;
        auto sent = m_connection->send(message.serialize(m_serializer.get()));
        if (sent.is_err()) LOG_ERROR("backend send: {}", sent.unwrap_err().toStdString());
    }

    void cancel(i32 request_id) { m_pending.cancel(request_id); }

    auto shutdown() -> std::shared_future<void> {
        m_shutting_down = true;
        m_connected     = false;
        fail_pending(QStringLiteral("backend shutting down"));
        return m_connection->shutdown();
    }

    void finish_shutdown() {
        m_connection.reset();
        if (m_process && m_process->state() != QProcess::NotRunning) {
            m_process->terminate();
            if (! m_process->waitForFinished(3000)) {
                m_process->kill();
                m_process->waitForFinished();
            }
        }
        m_process.reset();
        m_serializer.reset();
    }

private:
    void fail(const QString& message) {
        m_connected = false;
        fail_pending(message);
        if (m_connection) m_connection->disconnect();
        if (! m_shutting_down && ! std::exchange(m_failed, true)) Q_EMIT m_backend->error(message);
    }

    void fail_pending(const QString& message) { m_pending.fail_all(transport_error(message)); }

    Backend*                                    m_backend;
    std::unique_ptr<QProcess>                   m_process;
    std::unique_ptr<BackendConnection>          m_connection;
    std::unique_ptr<QProtobufSerializer>        m_serializer;
    BackendPending<msg::QcmMessage, msg::Error> m_pending;
    QMetaObject::Connection                     m_stdout_connection;
    i32                                         m_port { 0 };
    bool                                        m_shutting_down { false };
    bool                                        m_connected { false };
    bool                                        m_failed { false };
};

class RequestGuard {
public:
    RequestGuard(BackendTransport* transport, i32 request_id)
        : m_transport(transport), m_request_id(request_id) {}

    RequestGuard(const RequestGuard&)            = delete;
    RequestGuard& operator=(const RequestGuard&) = delete;

    ~RequestGuard() {
        auto transport = m_transport.data();
        if (transport == nullptr) return;
        (void)QMetaObject::invokeMethod(
            transport,
            [transport = QPointer { transport }, request_id = m_request_id] {
                if (transport == nullptr) return;
                transport->cancel(request_id);
            },
            Qt::QueuedConnection);
    }

private:
    QPointer<BackendTransport> m_transport;
    i32                        m_request_id;
};

} // namespace detail

Backend::Backend()
    : m_thread(Box<QThread>::make()),
      m_transport(new detail::BackendTransport(this)),
      m_serial(1),
      m_port(0) {
    m_transport->moveToThread(m_thread.get());
    connect(m_thread.get(), &QThread::finished, m_transport, &QObject::deleteLater);
    m_thread->start();
    (void)QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport] {
            transport->initialize();
        },
        Qt::QueuedConnection);

    connect(this, &Backend::started, this, &Backend::on_started);
    connect(this, &Backend::connected, this, &Backend::on_connected);
    connect(this, &Backend::error, this, &Backend::on_error);
    connect(this, &Backend::disconnected, this, [this] {
        m_resource_ready = false;
        Q_EMIT resourceStateChanged();
    });
}

Backend::~Backend() {
    std::shared_future<void> closed;
    (void)QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport, &closed] {
            closed = transport->shutdown();
        },
        Qt::BlockingQueuedConnection);
    try {
        closed.get();
    } catch (const std::exception& error) {
        LOG_ERROR("backend shutdown: {}", error.what());
    }
    (void)QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport] {
            transport->finish_shutdown();
        },
        Qt::BlockingQueuedConnection);
    m_thread->quit();
    m_thread->wait();
}

auto Backend::start(QStringView exe, QStringView data_dir, QStringView cache_dir) -> bool {
    m_exe       = exe.toString();
    m_data_dir  = data_dir.toString();
    m_cache_dir = cache_dir.toString();

    std::error_code error_code;
    auto            path = std::filesystem::path(m_exe.toStdString());
    if (! std::filesystem::exists(path, error_code)) {
        Q_EMIT error(QStringLiteral("Not found:\n %1").arg(m_exe));
        return false;
    }

    return QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport, exe = m_exe, data_dir = m_data_dir, cache_dir = m_cache_dir] {
            transport->start(exe, data_dir, cache_dir);
        },
        Qt::QueuedConnection);
}

void Backend::on_retry() { (void)start(m_exe, m_data_dir, m_cache_dir); }

void Backend::on_error(QString) {
    m_resource_ready = false;
    Q_EMIT resourceStateChanged();
    (void)QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport] {
            transport->kill();
        },
        Qt::QueuedConnection);
}

void Backend::on_started(i32 port) {
    m_resource_ready = false;
    Q_EMIT resourceStateChanged();
    m_port   = port;
    auto url = rstd::format("http://127.0.0.1:{}", port);
    (void)QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport, url = rstd::move(url), port]() mutable {
            transport->connect_to(rstd::move(url), port);
        },
        Qt::QueuedConnection);
}

void Backend::on_connected(i32) {
    m_resource_ready = true;
    Q_EMIT resourceStateChanged();
    auto message = msg::QcmMessage();
    message.setType(msg::MessageTypeGadget::MessageType::TEST_REQ);
    message.setTestReq({});
    send_immediate(rstd::move(message));
}

void Backend::send_immediate(msg::QcmMessage&& message) {
    message.setId_proto(serial());
    (void)QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport, message = rstd::move(message)]() mutable {
            transport->send_untracked(rstd::move(message));
        },
        Qt::QueuedConnection);
}

auto Backend::send(msg::QcmMessage&& message) -> task<Result<msg::QcmMessage, msg::Error>> {
    auto request_id = serial();
    message.setId_proto(request_id);

    auto completion_result = rstd::async::Completion<msg::QcmMessage, msg::Error>::make();
    if (completion_result.is_err()) {
        co_return Err(
            msg::Error { .code = -1, .message = "failed to allocate request completion state" });
    }

    auto pair       = rstd::move(completion_result).unwrap_unchecked();
    auto completion = rstd::move(pair.get<0>());
    auto response   = rstd::move(pair.get<1>());

    auto posted = QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport,
         message   = rstd::move(message),
         response  = rstd::move(response)]() mutable {
            transport->send(rstd::move(message), rstd::move(response));
        },
        Qt::QueuedConnection);
    if (! posted) {
        co_return Err(msg::Error { .code = -1, .message = "backend transport is unavailable" });
    }

    auto guard  = detail::RequestGuard { m_transport, request_id };
    auto result = co_await rstd::move(completion);
    if (result.is_err()) {
        auto error = rstd::move(result).unwrap_err_unchecked();
        if (error.is_failed()) co_return Err(rstd::move(error).unwrap_failed());
        co_return Err(msg::Error { .code = -1, .message = "request canceled" });
    }
    co_return Ok(rstd::move(result).unwrap_unchecked());
}

auto Backend::base() const -> std::string { return std::format("http://127.0.0.1:{}", m_port); }

auto Backend::resource_base_url() const -> QUrl {
    return m_resource_ready ? QUrl(QString::fromStdString(base())) : QUrl();
}
auto Backend::cache_directory() const -> QString { return m_cache_dir; }
auto Backend::image_url(model::ItemId id, enums::ImageType image_type) const -> QUrl {
    if (! m_resource_ready || id.id() < 0) return {};
    auto type = id.type();
    if (type == enums::ItemType::ItemAlbumArtist) type = enums::ItemType::ItemArtist;
    return QUrl(
        qextra::to_qstring(rstd::format("{}/image/{}/{}/{}", base(), type, id.id(), image_type)));
}

auto Backend::audio_url(model::ItemId id) -> QUrl {
    return QUrl(qextra::to_qstring(rstd::format("{}/audio/{}/{}", base(), id.type(), id.id())));
}
auto Backend::playback_url(model::ItemId id) -> QUrl {
    return QUrl(qextra::to_qstring(rstd::format("{}/playback/source/{}/{}", base(), id.type(), id.id())));
}

auto Backend::serial() -> i32 {
    i32 current = m_serial.load();
    for (;;) {
        auto next = current == std::numeric_limits<i32>::max() ? 1 : current + 1;
        if (m_serial.compare_exchange_strong(current, next)) return current;
    }
}

} // namespace qcm

#include "Qcm/backend.moc"
