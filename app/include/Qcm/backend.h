#pragma once

#include <map>
#include <functional>

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QThread>
#include <QProtobufSerializer>

#include <asio/thread_pool.hpp>
#include "asio_qt/qt_execution_context.h"
#include "core/asio/task.h"

#include "Qcm/backend_msg.h"

import qcm.core;
import ncrequest;
import rstd.rc;

namespace qcm
{

namespace detail
{
class BackendHelper;
} // namespace detail
class Backend : public QObject {
    Q_OBJECT

    friend class detail::BackendHelper;

public:
    Backend();
    ~Backend();

    auto start(QStringView exe, QStringView data_dir) -> bool;

    void send_immediate(msg::QcmMessage&& msg);

    auto send(msg::QcmMessage&& msg) -> task<Result<msg::QcmMessage, msg::Error>>;

    template<typename Req>
        requires msg::ReqMsgCP<Req>
    auto send(Req&& req) -> task<Result<typename msg::MsgTraits<Req>::Rsp, msg::Error>> {
        using Rsp = typename msg::MsgTraits<Req>::Rsp;
        auto msg  = msg::QcmMessage();
        msg::MsgTraits<Req>::set(msg, std::forward<Req>(req));
        co_return (co_await send(std::move(msg))).and_then([](auto m) -> Result<Rsp, msg::Error> {
            if (m.hasRsp()) {
                if (m.rsp().code() != msg::ErrorCodeGadget::ErrorCode::OK) {
                    return Err(msg::Error { .code    = (i32)m.rsp().code(),
                                            .message = m.rsp().message().toStdString() });
                }
            }
            return msg::get_rsp<Req>(m).ok_or(msg::Error {});
        });
    }

    Q_SIGNAL void started(i32 port);
    Q_SIGNAL void connected(i32 port);

private:
    Q_SLOT void on_started(i32 port);
    Q_SLOT void on_connected(i32 port);

    auto serial() -> i32;

    Box<QThread>                    m_thread;
    Box<QtExecutionContext>         m_context;
    QProcess*                       m_process;
    Box<ncrequest::WebSocketClient> m_client;
    Box<QProtobufSerializer>        m_serializer;

    std::map<i32, std::move_only_function<void(asio::error_code, msg::QcmMessage)>> m_handlers;

    std::atomic<i32> m_serial;
    i32              m_port;
};
} // namespace qcm