#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QSurfaceFormat>
#include <QLoggingCategory>
#include <QDir>
#include <QThread>

#include "Qcm/app.hpp"
#include "core/log.h"
import platform;
import qcm.log;

#include <QtQml/QQmlExtensionPlugin>
Q_IMPORT_QML_PLUGIN(Qcm_AppPlugin)

#include <kdsingleapplication.h>

namespace qcm
{
void qt_log(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    auto level = LogLevel::INFO;
    switch (type) {
    case QtMsgType::QtCriticalMsg:
    case QtMsgType::QtFatalMsg: {
        level = LogLevel::ERROR;
        break;
    }
    case QtMsgType::QtDebugMsg: {
        level = LogLevel::DEBUG;
        break;
    }
    case QtMsgType::QtWarningMsg: {
        level = LogLevel::WARN;
        break;
    }
    default: break;
    }
    std::string content;
    if (auto b = QByteArrayView(context.category); b != "default") {
        content = QString("[%1] %2").arg(b).arg(msg).toStdString();
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
} // namespace qcm

int main(int argc, char* argv[]) {
    plt::malloc_init();
    auto logger = qcm::LogManager::instance();
    ncrequest::global_init();

    qInstallMessageHandler(qcm::qt_log);

    // set by qml_material
    // qputenv("QT_QUICK_FLICKABLE_WHEEL_DECELERATION", "5000");
    // qputenv("QSGCURVEGLYPHATLAS_FONT_SIZE", "64");

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
        logger->set_level(qcm::log::level_from(parser.value(log_level_opt).toStdString()));

        backend_exe = parser.value(backend_opt);
        {
            auto path = QDir(backend_exe);
            if (! path.isAbsolute()) {
                backend_exe = QDir(QCoreApplication::applicationDirPath()).filePath(backend_exe);
            }
        }
        QLoggingCategory::setFilterRules(
            QLatin1String(std::format("qcm.debug={}", logger->level() == qcm::LogLevel::DEBUG)));
    }

    KDSingleApplication single;
    if (! single.isPrimaryInstance()) {
        LOG_WARN("another qcm running, triggering");
        single.sendMessageWithTimeout("hello", 5);
        exit(0);
    }

    int re { 0 };
    {
        qcm::App app { backend_exe, {} };
        QObject::connect(&single, &KDSingleApplication::messageReceived, app.instance(), []() {
            emit qcm::App::instance() -> instanceStarted();
        });
        app.init();

        re = gui_app.exec();
        main_qthread->setProperty("exec", false);
    }

    return re;
}
