#pragma once

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QThread>
#include <QProtobufSerializer>

#include <asio/thread_pool.hpp>
#include "asio_qt/qt_execution_context.h"

#include "Qcm/message/message.qpb.h"

import qcm.core;
import ncrequest;
import rstd.rc;

namespace qcm
{
class Backend : public QObject {
    Q_OBJECT
public:
    Backend();
    ~Backend();

    auto start(QStringView exe, QStringView data_dir) -> bool;

    Q_SIGNAL void started(i32 port);
    Q_SIGNAL void connected(i32 port);

private:
    Q_SLOT void on_started(i32 port);
    Q_SLOT void on_connected(i32 port);

    Box<QThread>                    m_thread;
    Box<QtExecutionContext>         m_context;
    QProcess*                       m_process;
    Box<ncrequest::WebSocketClient> m_client;
    Box<QProtobufSerializer>        m_serializer;

    i32 m_port;
};
} // namespace qcm