#include "Qcm/backend.hpp"

#include <filesystem>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMetaEnum>
#include <QtCore/QTextStream>

#include <asio/posix/stream_descriptor.hpp>

#include "core/log.h"
#include "core/qstr_helper.h"
#include "Qcm/status/process.hpp"
#include "Qcm/store.hpp"

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
        log::error("{}", err);
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
                if (msg.type() != msg::MessageTypeGadget::MessageType::PROVIDER_SYNC_STATUS_MSG) {
                    log::info("ws recv: {}", msg.type());
                }

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
            QTextStream                           out { stdout, QIODevice::WriteOnly };
            rstd::Option<QMetaObject::Connection> cnn;
        };
        auto state = rstd::rc::make_rc<State>();
        state->cnn = Some(connect(
            m_process, &QProcess::readyReadStandardOutput, m_process, [this, state] mutable {
                m_process->setReadChannel(QProcess::ProcessChannel::StandardOutput);
                if (m_process->canReadLine()) {
                    auto line = m_process->readLine();
                    auto doc  = QJsonDocument::fromJson(line);
                    if (auto jport = doc.object().value("port"); ! jport.isUndefined()) {
                        auto port = jport.toVariant().value<i32>();
                        log::info("backend port: {}", port);
                        Q_EMIT this->started(port);
                    } else {
                        log::error("read port from backend failed");
                    }
                    QObject::disconnect(*(state->cnn));
                    state->cnn = rstd::None();
                    QObject::connect(m_process,
                                     &QProcess::readyReadStandardOutput,
                                     m_process,
                                     [p = m_process, state] mutable {
                                         p->setReadChannel(
                                             QProcess::ProcessChannel::StandardOutput);
                                         bool flush = p->canReadLine();
                                         state->out << p->readAllStandardOutput();
                                         if (flush) state->out.flush();
                                     });
                }
            }));

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
            log::error("{}", ec.message());
            return false;
        }
    }

    m_context->post([this, exe, data_dir] {
        log::info("starting backend: {}", exe);
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

auto Backend::image(QStringView item_type, QStringView id, QStringView image_type)
    -> task<Arc<ncrequest::Response>> {
    auto url = std::format("{0}/image/{1}/{2}/{3}", this->base(), item_type, id, image_type);
    auto req = ncrequest::Request { url };

    co_return (co_await m_session->get(req)).unwrap();
}
auto Backend::image(model::ItemId id, enums::ImageType image_type)
    -> task<Arc<ncrequest::Response>> {
    auto url = std::format("{0}/image/{1}/{2}/{3}", this->base(), id.type(), id.id(), image_type);
    auto req = ncrequest::Request { url };

    co_return (co_await m_session->get(req)).unwrap();
}

auto Backend::audio_url(model::ItemId id) -> QUrl {
    return rstd::into(std::format("{0}/audio/{1}/{2}", this->base(), id.type(), id.id()));
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

void msg::merge_extra(QQmlPropertyMap& extra, const google::protobuf::Struct& in,
                      const std::set<QStringView>& is_json_field) {
    auto it  = in.fields().cbegin();
    auto end = in.fields().cend();
    for (; it != end; it++) {
        auto     key = it.key();
        QVariant val;
        if (is_json_field.contains(key)) {
            if (it.value().hasStringValue()) {
                auto json = QJsonDocument::fromJson(it.value().stringValue().toUtf8());
                val       = json.toVariant();
            } else {
                log::warn("wrong field");
            }
        } else {
            val = rstd::into(it.value());
        }
        extra.insert(key, std::move(val));
    }
}
} // namespace qcm

auto rstd::Impl<rstd::convert::From<google::protobuf::Value>, QVariant>::from(
    google::protobuf::Value val) -> QVariant {
    using KindFields = google::protobuf::Value::KindFields;
    switch (val.kindField()) {
    case KindFields::ListValue: {
        QVariantList list;
        for (auto& el : val.listValue().values()) {
            list.push_back(rstd::into(el));
        }
        return list;
    }
    case KindFields::StructValue: {
        QVariantMap map;
        auto        it  = val.structValue().fields().cbegin();
        auto        end = val.structValue().fields().cend();
        for (; it != end; it++) {
            map.insert(it.key(), rstd::into(it.value()));
        }
        return map;
    }
    case KindFields::NumberValue: {
        return val.numberValue();
    }
    case KindFields::StringValue: {
        return val.stringValue();
    }
    case KindFields::BoolValue: {
        return val.boolValue();
    }
    case KindFields::NullValue:
    default: {
        return {};
    }
    }
}

namespace qcm::model
{

auto Song::albumName() const -> QString {
    auto ex = AppStore::instance()->extra(itemId());
    if (ex) {
        auto al  = ex->value("album");
        auto map = al.toMap();
        return map.value("name", {}).toString();
    }
    return {};
}
} // namespace qcm::model
#include <Qcm/moc_backend_msg.cpp>
#include <Qcm/moc_backend.cpp>