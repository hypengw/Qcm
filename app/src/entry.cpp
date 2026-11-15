module;

#include <QCommandLineParser>
#include <QDeadlineTimer>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>
#include <QThread>
#include <QTimer>
#include <memory>

#ifdef Q_OS_LINUX
#    include <QDBusConnection>
#    include <QDBusMessage>
#    include <QDBusReply>
#endif

#include <QtQml/QQmlExtensionPlugin>

Q_IMPORT_QML_PLUGIN(Qcm_AppPlugin)
Q_IMPORT_QML_PLUGIN(Qcm_MsgPlugin)
Q_IMPORT_QML_PLUGIN(QExtraPlugin)

#include "core/log.h"
#include "Qcm/message/item_id.hpp"

module qcm.entry;

import platform;
import qcm;
import qcm.log;
import qcm.activation;

using rstd::boxed::Box;
using namespace Qt::Literals::StringLiterals;

namespace
{
auto discover_backend_port() -> quint16 {
#ifdef Q_OS_LINUX
    auto request = QDBusMessage::createMethodCall(u"io.github.hypengw.Qcm.Daemon"_s,
                                                  u"/io/github/hypengw/Qcm/Daemon"_s,
                                                  u"io.github.hypengw.Qcm.Daemon"_s,
                                                  u"GetPort"_s);
    request.setAutoStartService(false);
    const auto     bus = QDBusConnection::sessionBus();
    QDeadlineTimer deadline(5000);
    while (! deadline.hasExpired()) {
        QDBusReply<quint16> response =
            bus.call(request, QDBus::Block, static_cast<int>(deadline.remainingTime()));
        if (! response.isValid()) {
            qCritical("Cannot discover running backend: %s",
                      qPrintable(response.error().message()));
            return 0;
        }
        if (response.value() != 0) return response.value();
        QThread::msleep(100);
    }
    qCritical("Running backend did not become ready before discovery timed out");
#else
    qCritical("qcm-ui requires --port on this platform; start qcm instead");
#endif
    return 0;
}
} // namespace

namespace qcm
{
void qt_log(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    auto level = LogLevel::INFO;
    switch (type) {
    case QtMsgType::QtCriticalMsg:
    case QtMsgType::QtFatalMsg: level = LogLevel::ERROR; break;
    case QtMsgType::QtDebugMsg: level = LogLevel::DEBUG; break;
    case QtMsgType::QtWarningMsg: level = LogLevel::WARN; break;
    default: break;
    }

    std::string content;
    if (auto category = QByteArrayView(context.category); category != "default") {
        content = QString("[%1] %2").arg(category).arg(msg).toStdString();
    } else {
        content = msg.toStdString();
    }

    log::log_raw(
        level,
        log::log_format(level,
                        content,
                        context.file ? std::string_view(context.file) : std::string_view {},
                        context.line,
                        0));
}

int run(int argc, char** argv) {
    plt::malloc_init();
    auto logger = LogManager::instance();

    qInstallMessageHandler(qt_log);

    const auto      activation_token = activation::take_token();
    QGuiApplication gui_app(argc, argv);
    model::registerIdConverters();
    auto    main_qthread = gui_app.thread();
    quint16 backend_port {};
    QString cache_dir;

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setOrganizationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VERSION);

    {
        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addVersionOption();

        QCommandLineOption log_level_opt(
            "log-level", "Log Level (debug, info, warn, error)", "level", "warn");
        QCommandLineOption port_opt({ "p", "port" }, "Backend port supplied by qcm", "port");
        QCommandLineOption cache_opt(
            "cache",
            "Backend cache directory",
            "directory",
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

        parser.addOption(log_level_opt);
        parser.addOption(port_opt);
        parser.addOption(cache_opt);
        parser.process(gui_app);
        logger->set_level(log::level_from(parser.value(log_level_opt).toStdString()));

        if (parser.isSet(port_opt)) {
            bool valid_port {};
            auto value = parser.value(port_opt).toUInt(&valid_port);
            if (! valid_port || value == 0 || value > 65535) {
                qCritical("qcm-ui requires a valid --port; start qcm instead");
                return 2;
            }
            backend_port = static_cast<quint16>(value);
        } else {
            backend_port = discover_backend_port();
            if (! backend_port) return 2;
        }
        cache_dir = parser.value(cache_opt);
        QLoggingCategory::setFilterRules(
            QLatin1String(std::format("qcm.debug={}", logger->level() == LogLevel::DEBUG)));
    }

    int result { 0 };
    {
        auto                 backend   = Box<Backend>::make(activation_token);
        auto                 candidate = backend.as_mut_ptr();
        std::unique_ptr<App> app;
        bool                 admission_decided {};
        QTimer               startup_timer;
        startup_timer.setSingleShot(true);
        QObject::connect(&startup_timer, &QTimer::timeout, &gui_app, [&] {
            qCritical("Backend UI admission timed out");
            gui_app.exit(1);
        });
        QObject::connect(candidate, &Backend::error, &gui_app, [&](const QString& error) {
            if (app || admission_decided) return;
            startup_timer.stop();
            qCritical("Backend UI admission failed: %s", qPrintable(error));
            gui_app.exit(1);
        });
        QObject::connect(candidate, &Backend::disconnected, &gui_app, [&] {
            if (app || admission_decided) return;
            startup_timer.stop();
            qCritical("Backend disconnected before UI admission");
            gui_app.exit(1);
        });
        QObject::connect(
            candidate,
            &Backend::uiAdmission,
            &gui_app,
            [&](bool accepted) {
                startup_timer.stop();
                admission_decided = true;
                if (! accepted) {
                    gui_app.exit(0);
                    return;
                }
                if (app) return;
                app = std::make_unique<App>(std::move(backend), activation_token);
                app->init();
            },
            Qt::DirectConnection);
        startup_timer.start(10000);
        if (! candidate->connect_to(backend_port, cache_dir)) return 1;

        result = gui_app.exec();
        main_qthread->setProperty("exec", false);
    }

    return result;
}
} // namespace qcm
