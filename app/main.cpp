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

#include <QtQml/QQmlExtensionPlugin>
Q_IMPORT_QML_PLUGIN(Qcm_AppPlugin)

#include <kdsingleapplication.h>

int main(int argc, char* argv[]) {
    plt::malloc_init();
    auto logger = qcm::LogManager::instance();
    ncrequest::global_init();
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
        QCommandLineOption backend_opt(
            { "b", "backend" }, "backend executable path", "path", "QcmBackend");

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
        WARN_LOG("another qcm running, triggering");
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
