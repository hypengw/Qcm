module;
#include <cstdio>
#include <limits>
#include <map>
#include <memory>
#include <rstd/macro.hpp>

#include "Qcm/backend.moc.h"
#include "core/log.h"

module qcm;
import :backend;
import :status.process;
import ncrequest;
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

auto as_byte_array_view(rstd::slice<rstd::u8> bytes) -> QByteArrayView {
    return { reinterpret_cast<const char*>(bytes.as_raw_ptr()),
             static_cast<qsizetype>(bytes.len().to_primitive()) };
}

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
                    fail_pending(message);
                    if (! m_shutting_down) Q_EMIT m_backend->error(message);
                });
        connect(
            m_process.get(), &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
                auto key     = QMetaEnum::fromType<QProcess::ProcessError>().valueToKey(error);
                auto message = QStringLiteral("Backend %1").arg(QString::fromLatin1(key));
                fail_pending(message);
                if (! m_shutting_down) Q_EMIT m_backend->error(message);
            });

        m_serializer = std::make_unique<QProtobufSerializer>();
        m_client     = std::make_unique<ncrequest::WebSocketClient>();
        m_client->set_on_error_callback([this](rstd::ref<rstd::str> error) {
            auto message = QString::fromUtf8(reinterpret_cast<const char*>(error.data()),
                                             static_cast<qsizetype>(error.size().to_primitive()));
            LOG_ERROR("websocket: {}", message.toStdString());
            fail_pending(message);
            if (! m_shutting_down) Q_EMIT m_backend->error(message);
        });
        m_client->set_on_connected_callback([this] {
            Q_EMIT m_backend->connected(m_port);
        });
        m_client->set_on_disconnected_callback([this] {
            fail_pending(QStringLiteral("backend disconnected"));
        });
        m_client->set_on_message_callback(
            [this, cache = QByteArray {}](rstd::slice<rstd::u8> bytes, bool last) mutable {
                auto chunk = as_byte_array_view(bytes);
                if (! last) {
                    cache.append(chunk);
                    return;
                }

                msg::QcmMessage message;
                if (cache.isEmpty()) {
                    message.deserialize(m_serializer.get(), chunk);
                } else {
                    cache.append(chunk);
                    message.deserialize(m_serializer.get(), cache);
                    cache.clear();
                }

                if (message.type()
                    != msg::MessageTypeGadget::MessageType::PROVIDER_SYNC_STATUS_MSG) {
                    LOG_INFO("ws recv: {}", message.type());
                }

                if (auto it = m_handlers.find(message.id_proto()); it != m_handlers.end()) {
                    (void)it->second.complete(rstd::move(message));
                    m_handlers.erase(it);
                    return;
                }

                (void)QMetaObject::invokeMethod(
                    m_backend,
                    [message = rstd::move(message)]() mutable {
                        qcm::process_msg(rstd::move(message));
                    },
                    Qt::QueuedConnection);
            });
    }

    void start(QString exe, QString data_dir, QString cache_dir) {
        if (! m_process || m_process->state() != QProcess::NotRunning) return;

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
        if (m_process && m_process->state() != QProcess::NotRunning) m_process->kill();
    }

    void connect_to(rstd::string::String url, i32 port) {
        m_port = port;
        if (m_client) (void)m_client->connect(url.as_str());
    }

    void send(msg::QcmMessage message, ResponseHandle response) {
        if (! m_client || ! m_client->is_connected()) {
            (void)response.fail(transport_error(QStringLiteral("backend disconnected")));
            return;
        }

        auto id = message.id_proto();
        m_handlers.insert_or_assign(id, rstd::move(response));
        send_untracked(rstd::move(message));
    }

    void send_untracked(msg::QcmMessage message) {
        if (! m_client || ! m_client->is_connected()) return;
        auto bytes = message.serialize(m_serializer.get());
        m_client->send(rstd::slice<rstd::u8>::from_raw_parts(
            reinterpret_cast<const rstd::byte*>(bytes.constData()),
            rstd::usize(static_cast<std::size_t>(bytes.size()))));
    }

    void cancel(i32 request_id) { m_handlers.erase(request_id); }

    void shutdown() {
        m_shutting_down = true;
        fail_pending(QStringLiteral("backend shutting down"));
        if (m_client) {
            m_client->disconnect();
            m_client.reset();
        }

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
    void fail_pending(const QString& message) {
        for (auto& [id, handler] : m_handlers) {
            (void)id;
            (void)handler.fail(transport_error(message));
        }
        m_handlers.clear();
    }

    Backend*                                    m_backend;
    std::unique_ptr<QProcess>                   m_process;
    std::unique_ptr<ncrequest::WebSocketClient> m_client;
    std::unique_ptr<QProtobufSerializer>        m_serializer;
    std::map<i32, ResponseHandle>               m_handlers;
    QMetaObject::Connection                     m_stdout_connection;
    i32                                         m_port { 0 };
    bool                                        m_shutting_down { false };
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

Backend::Backend(Arc<ncrequest::Session> session)
    : m_thread(Box<QThread>::make()),
      m_transport(new detail::BackendTransport(this)),
      m_session(rstd::move(session)),
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
}

Backend::~Backend() {
    (void)QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport] {
            transport->shutdown();
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
    (void)QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport] {
            transport->kill();
        },
        Qt::QueuedConnection);
}

void Backend::on_started(i32 port) {
    m_port   = port;
    auto url = rstd::format("ws://127.0.0.1:{}", port);
    (void)QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport, url = rstd::move(url), port]() mutable {
            transport->connect_to(rstd::move(url), port);
        },
        Qt::QueuedConnection);
}

void Backend::on_connected(i32) {
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

auto Backend::image(QStringView item_type, QStringView id, QStringView image_type)
    -> Result<ncrequest::Request, msg::Error> {
    auto url     = rstd::format("{}/image/{}/{}/{}", base(), item_type, id, image_type);
    auto request = ncrequest::Request::from_url(url.as_str());
    if (request.is_err()) {
        auto error = rstd::move(request).unwrap_err_unchecked();
        return Err(msg::Error {
            .code    = -1,
            .message = std::format("invalid image URL at byte {}", error.offset().to_primitive()),
        });
    }
    return Ok(rstd::move(request).unwrap_unchecked());
}

auto Backend::image(model::ItemId id, enums::ImageType image_type)
    -> Result<ncrequest::Request, msg::Error> {
    auto type = id.type();
    if (type == enums::ItemType::ItemAlbumArtist) type = enums::ItemType::ItemArtist;
    auto url     = rstd::format("{}/image/{}/{}/{}", base(), type, id.id(), image_type);
    auto request = ncrequest::Request::from_url(url.as_str());
    if (request.is_err()) {
        auto error = rstd::move(request).unwrap_err_unchecked();
        return Err(msg::Error {
            .code    = -1,
            .message = std::format("invalid image URL at byte {}", error.offset().to_primitive()),
        });
    }
    return Ok(rstd::move(request).unwrap_unchecked());
}

auto Backend::audio_url(model::ItemId id) -> QUrl {
    return QUrl(qextra::to_qstring(rstd::format("{}/audio/{}/{}", base(), id.type(), id.id())));
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
