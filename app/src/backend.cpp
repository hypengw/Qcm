module;
#include <exception>
#include <future>
#include <limits>
#include <memory>
#include <rstd/macro.hpp>

#include "Qcm/backend.moc.h"
#include "Qcm/message/load_pool.hpp"
#include "Qcm/message/protocol.hpp"
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

class EntityLoader : public QObject {
public:
    using Callback = std::function<void(const Backend::SongResult&)>;
    struct Flight {
        QPointer<Query>         operation;
        QList<i64>              ids;
        QMap<quint64, Callback> consumers;
        ~Flight() {
            if (operation) {
                operation->cancel();
                operation->deleteLater();
            }
            for (auto id : ids)
                AppStore::instance()->unpin(model::ItemId(enums::ItemType::ItemSong, id));
        }
    };
    explicit EntityLoader(Backend* backend): QObject(backend), m_backend(backend) {}
    StoreSubscription load(QString key, QList<i64> ids, QObject* consumer, Callback callback) {
        if (ids.isEmpty()) return {};
        auto [flight, leader] = m_flights.acquire(key, [&] {
            auto value       = std::make_shared<Flight>();
            value->ids       = ids;
            value->operation = new Query(this);
            for (auto id : ids)
                AppStore::instance()->pin(model::ItemId(enums::ItemType::ItemSong, id));
            return value;
        });
        if (! flight) {
            callback(Err(msg::Error { .code = -1, .message = "entity loader capacity exceeded" }));
            return {};
        }
        const auto        ticket = ++m_serial;
        QPointer<QObject> observer(consumer);
        flight->consumers.insert(ticket,
                                 [observer, callback = std::move(callback)](const auto& result) {
                                     if (observer) callback(result);
                                 });
        auto weak  = QPointer<EntityLoader>(this);
        auto leave = [weak, key, ticket, expected = std::weak_ptr<Flight>(flight)] {
            if (! weak) return;
            auto flight = expected.lock();
            if (! flight) return;
            flight->consumers.remove(ticket);
            if (flight->consumers.isEmpty()) (void)weak->m_flights.take(key, flight);
        };
        auto connection = connect(consumer, &QObject::destroyed, this, leave);
        if (leader) {
            auto backend = QPointer<Backend>(m_backend);
            flight->operation->spawn(
                [weak, backend, key, ids, expected = std::weak_ptr<Flight>(flight)]()
                    -> task<void> {
                    if (! backend) co_return;
                    msg::GetSongsByIdReq request;
                    request.setIds(identity::fromValues<msg::id::SongIds>(ids));
                    auto response = co_await backend->send(std::move(request));
                    if (! co_await QAsyncResult::qexecutor() || ! weak) co_return;
                    auto flight = weak->m_flights.take(key, expected.lock());
                    if (! flight) co_return;
                    const auto consumers = flight->consumers;
                    for (auto it = consumers.cbegin(); it != consumers.cend(); ++it) {
                        auto callback = it.value();
                        callback(response);
                    }
                    QTimer::singleShot(0, weak, [flight] {
                    });
                });
        }
        return StoreSubscription([weak, leave = std::move(leave), connection] {
            QObject::disconnect(connection);
            if (weak) {
                if (QThread::currentThread() == weak->thread())
                    leave();
                else
                    QMetaObject::invokeMethod(weak, leave, Qt::QueuedConnection);
            }
        });
    }
    void reset() {
        auto flights = m_flights.clear();
        auto error   = Backend::SongResult(
            Err(msg::Error { .code = -1, .message = "entity loader generation changed" }));
        for (const auto& [key, flight] : flights) {
            const auto consumers = flight->consumers;
            for (const auto& callback : consumers) callback(error);
        }
    }

private:
    Backend*                         m_backend;
    store::LoadPool<QString, Flight> m_flights;
    quint64                          m_serial {};
};

class BackendTransport : public QObject {
public:
    explicit BackendTransport(Backend* backend): m_backend(backend) {}

    void initialize() {
        plt::set_thread_name("backend");
        m_handshake_timer = new QTimer(this);
        m_handshake_timer->setSingleShot(true);
        connect(m_handshake_timer, &QTimer::timeout, this, [this] {
            fail(QStringLiteral("Backend handshake timed out"));
        });

        m_serializer = std::make_unique<QProtobufSerializer>();
        BackendConnectionCallbacks callbacks;
        callbacks.connected = [this] {
            reset_protocol();
            (void)m_backend->m_generation.fetch_add(1);
            m_connected = true;
            Q_EMIT m_backend->connected(m_port);
        };
        callbacks.error = [this](QString message) {
            fail(message);
        };
        callbacks.disconnected = [this] {
            reset_protocol();
            (void)m_backend->m_generation.fetch_add(1);
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
            if (auto error = m_protocol.accept(message); ! error.isEmpty()) {
                fail(error);
                return;
            }
            if (m_protocol.ready()) m_handshake_timer->stop();

            if (message.type() != msg::MessageTypeGadget::MessageType::PROVIDER_SYNC_STATUS_MSG) {
                LOG_INFO("ws recv: {}", message.type());
            }

            auto id = message.id_proto();
            if (id != 0 && ! message.hasTestRsp()) {
                (void)m_pending.complete(id, rstd::move(message));
                return;
            }

            (void)QMetaObject::invokeMethod(
                m_backend,
                [backend    = m_backend,
                 generation = m_backend->m_generation.load(),
                 message    = rstd::move(message)]() mutable {
                    if (generation != backend->m_generation.load()) return;
                    if (message.hasEntities()) {
                        auto batch = message.entities();
                        AppStore::instance()->deliver(
                            std::move(batch),
                            [backend = QPointer<Backend>(backend),
                             message = std::move(message)](QString error) mutable {
                                if (! backend) return;
                                if (! error.isEmpty()) {
                                    Q_EMIT backend->error(error);
                                    return;
                                }
                                if (message.hasTestRsp()) {
                                    backend->m_resource_ready = true;
                                    Q_EMIT backend->resourceStateChanged();
                                    backend->requestPlayerSnapshot();
                                }
                                qcm::process_msg(std::move(message));
                            });
                    } else if (message.hasTestRsp()) {
                        Q_EMIT backend->error(
                            QStringLiteral("Backend does not support entity protocol v2"));
                    } else
                        qcm::process_msg(std::move(message));
                },
                Qt::QueuedConnection);
        };
        m_connection = std::make_unique<BackendConnection>(QAsyncResult::runtime_handle(),
                                                           std::move(callbacks));
    }

    void connect_to(rstd::string::String url, i32 port) {
        if (m_shutting_down) return;
        m_failed    = false;
        m_connected = false;
        reset_protocol();
        m_connection->disconnect();
        m_port = port;
        m_connection->open(qextra::to_qstring(url));
    }

    void send(msg::QcmMessage message, ResponseHandle response) {
        if (! m_connection || ! m_connected || m_shutting_down) {
            (void)response.fail(transport_error(QStringLiteral("backend disconnected")));
            return;
        }
        if (! m_protocol.ready()) {
            (void)response.fail(transport_error(QStringLiteral("backend handshake not complete")));
            return;
        }
        message.setProtocolVersion(protocol::Version);

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
        if (! m_connection || ! m_connected || m_shutting_down) return;
        if (! m_protocol.ready()) {
            if (! message.hasTestReq()) {
                fail(QStringLiteral("backend handshake not complete"));
                return;
            }
            m_protocol.begin(message.id_proto());
            m_handshake_timer->start(5000);
        }
        message.setProtocolVersion(protocol::Version);
        auto sent = m_connection->send(message.serialize(m_serializer.get()));
        if (sent.is_err()) LOG_ERROR("backend send: {}", sent.unwrap_err().toStdString());
    }

    void cancel(i32 request_id) { m_pending.cancel(request_id); }

    auto shutdown() -> std::shared_future<void> {
        reset_protocol();
        m_shutting_down = true;
        m_connected     = false;
        fail_pending(QStringLiteral("backend shutting down"));
        return m_connection->shutdown();
    }

    void finish_shutdown() {
        m_connection.reset();
        m_serializer.reset();
    }

private:
    void reset_protocol() {
        m_protocol.reset();
        if (m_handshake_timer) m_handshake_timer->stop();
    }
    void fail(const QString& message) {
        LOG_ERROR("backend transport: {}", message.toStdString());
        reset_protocol();
        m_connected = false;
        fail_pending(message);
        if (m_connection) m_connection->disconnect();
        if (! m_shutting_down && ! std::exchange(m_failed, true)) Q_EMIT m_backend->error(message);
    }

    void fail_pending(const QString& message) { m_pending.fail_all(transport_error(message)); }

    Backend*                                    m_backend;
    protocol::Session                           m_protocol;
    QTimer*                                     m_handshake_timer {};
    std::unique_ptr<BackendConnection>          m_connection;
    std::unique_ptr<QProtobufSerializer>        m_serializer;
    BackendPending<msg::QcmMessage, msg::Error> m_pending;
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
      m_loader(std::make_unique<detail::EntityLoader>(this)),
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

    connect(this, &Backend::connected, this, &Backend::on_connected);
    connect(this, &Backend::error, this, &Backend::on_error);
    connect(this, &Backend::disconnected, this, [this] {
        m_loader->reset();
        AppStore::instance()->markStale();
        m_resource_ready = false;
        Q_EMIT resourceStateChanged();
    });
}

Backend::~Backend() {
    m_player_commands.cancel();
    m_loader->reset();
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

auto Backend::connect_to(i32 port, QStringView cache_dir) -> bool {
    m_port           = port;
    m_cache_dir      = cache_dir.toString();
    m_resource_ready = false;
    Q_EMIT resourceStateChanged();
    if (port < 1 || port > 65535) {
        Q_EMIT error(QStringLiteral("Invalid backend port"));
        return false;
    }
    auto url = rstd::format("http://127.0.0.1:{}", port);
    return QMetaObject::invokeMethod(
        m_transport,
        [transport = m_transport, url = rstd::move(url), port]() mutable {
            transport->connect_to(rstd::move(url), port);
        },
        Qt::QueuedConnection);
}

void Backend::on_retry() { (void)connect_to(m_port, m_cache_dir); }

void Backend::on_error(QString) {
    m_resource_ready = false;
    Q_EMIT resourceStateChanged();
}

void Backend::on_connected(i32) {
    m_loader->reset();
    AppStore::instance()->reset();
    m_resource_ready = false;
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

void Backend::controlPlayer(msg::PlayerControlReq request) {
    msg::QcmMessage message;
    msg::MsgTraits<msg::PlayerControlReq>::set(message, std::move(request));
    submitPlayerRequest(std::move(message));
}

void Backend::updatePlayerQueue(msg::PlayerQueueReq request, std::function<void(bool)> completed) {
    msg::QcmMessage message;
    msg::MsgTraits<msg::PlayerQueueReq>::set(message, std::move(request));
    submitPlayerRequest(std::move(message), std::move(completed));
}

void Backend::requestPlayerSnapshot() {
    msg::PlayerControlReq request;
    request.setAction(msg::PlayerActionGadget::PlayerAction::PLAYER_ACTION_SNAPSHOT);
    controlPlayer(std::move(request));
}

void Backend::submitPlayerRequest(msg::QcmMessage message, std::function<void(bool)> completed) {
    if (! m_resource_ready) {
        if (completed) completed(false);
        return;
    }
    auto       self       = QPointer<Backend>(this);
    const auto generation = m_generation.load();
    m_player_commands.spawn([self,
                             generation,
                             message   = std::move(message),
                             completed = std::move(completed)]() mutable -> task<void> {
        if (! self) co_return;
        auto result = co_await self->send(std::move(message));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self) co_return;
        if (generation != self->m_generation.load()) {
            if (completed) completed(false);
            co_return;
        }
        QString error;
        if (! result) {
            error = QString::fromStdString(result.unwrap_err().message);
        } else if (! result->hasRsp()) {
            error = QStringLiteral("Missing player command response");
        } else if (result->rsp().code() != msg::ErrorCodeGadget::ErrorCode::ERROR_CODE_OK) {
            error = result->rsp().message();
            if (error.isEmpty()) error = QStringLiteral("Player command failed");
        }
        if (completed) completed(error.isEmpty());
        if (! error.isEmpty()) Q_EMIT self->playerError(error);
    });
}

auto Backend::send(msg::QcmMessage&& message) -> task<Result<msg::QcmMessage, msg::Error>> {
    const auto self         = QPointer<Backend>(this);
    const auto generation   = m_generation.load();
    auto       request_id   = serial();
    const auto request_type = message.type();
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
    if (! co_await QAsyncResult::qexecutor())
        co_return Err(msg::Error { .code = -1, .message = "entity delivery canceled" });
    if (! self || generation != self->m_generation.load())
        co_return Err(msg::Error { .code = -1, .message = "backend generation changed" });
    auto received = rstd::move(result).unwrap_unchecked();
    if (received.hasEntities()) {
        auto state = rstd::async::Completion<rstd::empty, QString>::make();
        if (state.is_err())
            co_return Err(
                msg::Error { .code = -1, .message = "entity delivery allocation failed" });
        auto pair       = std::move(state).unwrap_unchecked();
        auto completion = std::move(pair.get<0>());
        auto response   = std::make_shared<rstd::async::CompletionHandle<rstd::empty, QString>>(
            std::move(pair.get<1>()));
        AppStore::instance()->deliver(received.entities(), [response](QString error) {
            if (error.isEmpty())
                (void)response->complete(rstd::empty {});
            else
                (void)response->fail(error);
        });
        auto applied = co_await std::move(completion);
        if (applied.is_err()) {
            auto error = std::move(applied).unwrap_err_unchecked();
            co_return Err(msg::Error {
                .code    = -1,
                .message = error.is_failed() ? std::move(error).unwrap_failed().toStdString()
                                             : "entity delivery canceled" });
        }
        if (! co_await QAsyncResult::qexecutor() ||
            (! self || generation != self->m_generation.load()))
            co_return Err(
                msg::Error { .code = -1, .message = "backend generation changed during delivery" });
    } else if (request_type != msg::MessageTypeGadget::MessageType::PLAYER_CONTROL_REQ &&
               request_type != msg::MessageTypeGadget::MessageType::PLAYER_QUEUE_REQ &&
               (! received.hasRsp() ||
                received.rsp().code() == msg::ErrorCodeGadget::ErrorCode::ERROR_CODE_OK)) {
        if (received.hasRsp()) {
            LOG_ERROR("entity protocol v1: type={} id={} code={} message={}",
                      request_type,
                      request_id,
                      static_cast<int>(received.rsp().code()),
                      received.rsp().message().toStdString());
        } else {
            LOG_ERROR("entity protocol v1: type={} id={} no rsp", request_type, request_id);
        }
        co_return Err(
            msg::Error { .code = -1, .message = "backend does not support entity protocol v1" });
    }
    co_return Ok(rstd::move(received));
}

StoreSubscription Backend::loadSongs(const msg::id::SongIds& input, QObject* consumer,
                                     std::function<void(const SongResult&)> callback) {
    Q_ASSERT(QThread::currentThread() == thread());
    if (! identity::valid(input)) {
        callback(Err(msg::Error { .code = -1, .message = "invalid song IDs" }));
        return {};
    }
    QList<i64> ids;
    ids.reserve(input.values().size());
    for (auto id : input.values()) ids.push_back(id);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    QStringList parts;
    parts.push_back(QString::number(m_generation.load()));
    parts.push_back(QString::number(AppStore::instance()->scopeRevision()));
    for (auto id : ids) parts.push_back(QString::number(id));
    return m_loader->load(parts.join(u':'), std::move(ids), consumer, std::move(callback));
}

auto Backend::base() const -> std::string { return std::format("http://127.0.0.1:{}", m_port); }

auto Backend::resource_base_url() const -> QUrl {
    return m_resource_ready ? QUrl(QString::fromStdString(base())) : QUrl();
}
auto Backend::cache_directory() const -> QString { return m_cache_dir; }
auto Backend::image_url(model::ItemId id, enums::ImageType image_type) const -> QUrl {
    if (! m_resource_ready || ! id.valid()) return {};
    auto type = id.type();
    if (type == enums::ItemType::ItemAlbumArtist) type = enums::ItemType::ItemArtist;
    return QUrl(
        qextra::to_qstring(rstd::format("{}/image/{}/{}/{}", base(), type, id.id(), image_type)));
}

auto Backend::audio_url(model::ItemId id) -> QUrl {
    return QUrl(qextra::to_qstring(rstd::format("{}/audio/{}/{}", base(), id.type(), id.id())));
}
auto Backend::playback_url(model::ItemId id) -> QUrl {
    return QUrl(
        qextra::to_qstring(rstd::format("{}/playback/source/{}/{}", base(), id.type(), id.id())));
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
