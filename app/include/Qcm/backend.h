#pragma once

#include <map>
#include <functional>

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QThread>
#include <QProtobufSerializer>

#include <asio/thread_pool.hpp>
#include "asio_qt/qt_execution_context.h"
#include "asio_helper/task.h"

#include "Qcm/message/message.qpb.h"

import qcm.core;
import ncrequest;
import rstd.rc;

namespace qcm
{

namespace detail
{
class BackendHelper;
}
class Backend : public QObject {
    Q_OBJECT

    friend class detail::BackendHelper;

public:
    Backend();
    ~Backend();

    auto start(QStringView exe, QStringView data_dir) -> bool;

    void send_immediate(msg::QcmMessage&& msg);

    auto send(msg::QcmMessage&& msg) -> task<msg::QcmMessage>;

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