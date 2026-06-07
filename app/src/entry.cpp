module;

#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>
#include <QThread>

#include <QtQml/QQmlExtensionPlugin>

Q_IMPORT_QML_PLUGIN(Qcm_AppPlugin)
Q_IMPORT_QML_PLUGIN(Qcm_MsgPlugin)

#include <kdsingleapplication.h>

#include "core/log.h"

module qcm.entry;

import ncrequest;
import platform;
import qcm;
import qcm.log;

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
    ncrequest::global_init();

    qInstallMessageHandler(qt_log);

    QGuiApplication gui_app(argc, argv);
    auto            main_qthread = gui_app.thread();
    QString         backend_exe;

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VERSION);

    {
        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addVersionOption();

        QCommandLineOption log_level_opt(
            "log-level", "Log Level (debug, info, warn, error)", "level", "warn");
        QCommandLineOption backend_opt({ "b", "backend" },
                                       "backend executable path",
                                       "path",
#ifdef _WIN32
                                       "QcmBackend.exe"
#else
                                       "QcmBackend"
#endif
        );

        parser.addOption(log_level_opt);
        parser.addOption(backend_opt);
        parser.process(gui_app);
        logger->set_level(log::level_from(parser.value(log_level_opt).toStdString()));

        backend_exe = parser.value(backend_opt);
        auto path   = QDir(backend_exe);
        if (! path.isAbsolute()) {
            backend_exe = QDir(QCoreApplication::applicationDirPath()).filePath(backend_exe);
        }
        QLoggingCategory::setFilterRules(
            QLatin1String(std::format("qcm.debug={}", logger->level() == LogLevel::DEBUG)));
    }

    KDSingleApplication single;
    if (! single.isPrimaryInstance()) {
        LOG_WARN("another qcm running, triggering");
        single.sendMessageWithTimeout("hello", 5);
        return 0;
    }

    int result { 0 };
    {
        App app { backend_exe, {} };
        QObject::connect(&single, &KDSingleApplication::messageReceived, app.instance(), []() {
            emit App::instance() -> instanceStarted();
        });
        app.init();

        result = gui_app.exec();
        main_qthread->setProperty("exec", false);
    }

    return result;
}
} // namespace qcm
