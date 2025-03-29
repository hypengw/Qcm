#include "Qcm/backend.hpp"

#include <filesystem>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaEnum>

#include <asio/posix/stream_descriptor.hpp>

#include "core/log.h"
#include "core/qstr_helper.h"
#include "Qcm/status/process.hpp"

import ncrequest.event;
import rstd.rc;
import platform;

namespace qcm
{

namespace detail
{
class BackendHelper {
public:
    template<typename CompletionToken>
    static auto async_send(Backend& backend, msg::QcmMessage&& msg, CompletionToken&& token) {
        using ret = void(asio::error_code, msg::QcmMessage);
        return asio::async_initiate<CompletionToken, ret>(
            [&](auto&& handler) {
                asio::dispatch(
                    backend.m_context->get_executor(),
                    [&backend, msg = std::move(msg), handler = std::move(handler)] mutable {
                        msg.setId_proto(backend.serial());
                        backend.m_handlers.insert_or_assign(
                            msg.id_proto(), std::move_only_function<ret> { std::move(handler) });
                        auto bytes = msg.serialize(backend.m_serializer.get());
                        backend.m_client->send({ bytes.constData(), (std::size_t)bytes.size() });
                    });
            },
            token);
    }
};
} // namespace detail

Backend::Backend(Arc<ncrequest::Session> session)
    : m_thread(make_box<QThread>(new QThread())),
      m_context(
          make_box<QtExecutionContext>(m_thread.get(), (QEvent::Type)QEvent::registerEventType())),
      m_process(new QProcess()),
      m_client(make_box<ncrequest::WebSocketClient>(
          ncrequest::event::create<asio::posix::basic_stream_descriptor>(
              m_context->get_executor()))),
      m_serializer(make_box<QProtobufSerializer>()),
      m_session(session),
      m_serial(1), // start from 1, as 0 is none
      m_port(0) {
    m_process->setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedErrorChannel);
    m_client->set_on_error_callback([](std::string_view err) {
        ERROR_LOG("{}", err);
    });
    m_client->set_on_connected_callback([this]() {
        Q_EMIT this->connected(m_port);
    });
    m_client->set_on_message_callback(
        [this, cache = make_rc<std::vector<byte>>()](std::span<const std::byte> bytes, bool last) {
            if (! last) {
                std::ranges::copy(bytes, std::back_inserter(*cache));
            } else {
                msg::QcmMessage msg;
                if (cache->empty()) {
                    msg.deserialize(m_serializer.get(), bytes);
                } else {
                    std::ranges::copy(bytes, std::back_inserter(*cache));
                    msg.deserialize(m_serializer.get(), *cache);
                    cache->clear();
                }
                log::info("ws recv: {}", msg.type());

                if (auto it = m_handlers.find(msg.id_proto()); it != m_handlers.end()) {
                    it->second(asio::error_code {}, std::move(msg));
                    m_handlers.erase(it);
                } else {
                    qcm::process_msg(std::move(msg));
                }
            }
        });
    // start thread
    {
        QObject::connect(m_thread.get(), &QThread::finished, m_process, &QObject::deleteLater);
        m_thread->start();
        bool ok = m_process->moveToThread(m_thread.get());
        _assert_(ok);
    }
    // connect signal
    {
        struct State {
            bool port_readed { false };
        };
        auto state = rstd::rc::make_rc<State>();
        connect(m_process, &QProcess::readyReadStandardOutput, m_process, [this, state] mutable {
            if (state->port_readed) return;

            m_process->setReadChannel(QProcess::ProcessChannel::StandardOutput);
            if (m_process->canReadLine()) {
                auto line = m_process->readLine();
                if (! state->port_readed) {
                    state->port_readed = true;
                    auto doc           = QJsonDocument::fromJson(line);
                    if (auto jport = doc.object().value("port"); ! jport.isUndefined()) {
                        auto port = jport.toVariant().value<i32>();
                        INFO_LOG("backend port: {}", port);
                        Q_EMIT this->started(port);
                    } else {
                        ERROR_LOG("read port from backend failed");
                    }
                } else {
                    INFO_LOG("{}", QString(line));
                }
            }
        });

        connect(this, &Backend::started, this, &Backend::on_started);
        connect(this, &Backend::connected, this, &Backend::on_connected);
    }
    {
        asio::post(m_context->get_executor(), [] {
            plt::set_thread_name("ws");
        });
    }
}

Backend::~Backend() {
    QMetaObject::invokeMethod(m_process, [self = m_process] {
        self->terminate();
    });
    m_thread->quit();
    m_thread->wait();
}

auto Backend::start(QStringView exe_, QStringView data_dir_) -> bool {
    auto exe      = exe_.toString();
    auto data_dir = data_dir_.toString();
    {
        std::error_code ec;
        auto            path = std::filesystem::path(exe.toStdString());
        if (! std::filesystem::exists(path, ec)) {
            ERROR_LOG("{}", ec.message());
            return false;
        }
    }

    m_context->post([this, exe, data_dir] {
        INFO_LOG("starting backend: {}", exe);
        m_process->start(exe, { u"--data"_s, data_dir });
    });
    return true;
}

void Backend::on_started(i32 port) {
    m_port = port;
    m_client->connect(std::format("ws://127.0.0.1:{}", port));
}

void Backend::on_connected(i32) {
    auto msg = msg::QcmMessage();
    msg.setType(msg::MessageTypeGadget::MessageType::TEST_REQ);
    msg.setTestReq({});
    send_immediate(std::move(msg));
}

void Backend::send_immediate(msg::QcmMessage&& msg) {
    msg.setId_proto(serial());
    auto bytes = msg.serialize(m_serializer.get());
    m_client->send({ bytes.constData(), (std::size_t)bytes.size() });
}

auto Backend::send(msg::QcmMessage&& msg) -> task<Result<msg::QcmMessage, msg::Error>> {
    auto [ec, var] =
        co_await detail::BackendHelper::async_send(*this, std::move(msg), qcm::as_tuple(use_task));
    co_return Ok(var);
}

auto Backend::base() const -> std::string { return std::format("http://127.0.0.1:{}", m_port); }

auto Backend::image(QStringView library_id, QStringView item_id, QStringView image_id)
    -> task<Arc<ncrequest::Response>> {
    auto url = std::format("{0}/image/{1}/{2}/{3}", this->base(), library_id, item_id, image_id);
    auto req = ncrequest::Request { url };

    co_return (co_await m_session->get(req)).unwrap();
}

auto Backend::serial() -> i32 {
    i32 cur = m_serial.load();
    for (;;) {
        const i32 to = (cur + 1) % std::numeric_limits<i32>::max();
        if (m_serial.compare_exchange_strong(cur, to)) {
            break;
        }
    }
    return cur;
}
} // namespace qcm

#include <Qcm/moc_backend.cpp>