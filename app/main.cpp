#include <QApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QSurfaceFormat>
#include <QLoggingCategory>
#include <QThread>

#include "Qcm/app.h"
#include "request/request.h"
#include "core/log.h"
#include "platform/platform.h"

#include <QtQml/QQmlExtensionPlugin>
Q_IMPORT_QML_PLUGIN(Qcm_AppPlugin)

#include <kdsingleapplication.h>

int main(int argc, char* argv[]) {
    plt::malloc_init();

    auto logger = qcm::LogManager::init();
    request::global_init();

    QApplication gui_app(argc, argv);

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VERSION);

    {
        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addVersionOption();
        QCommandLineOption verboseOption("verbose");
        parser.addOption(verboseOption);
        parser.process(gui_app);

        logger->set_level(parser.isSet(verboseOption) ? qcm::LogLevel::DEBUG : qcm::LogLevel::WARN);
        QLoggingCategory::setFilterRules(
            QLatin1String(fmt::format("qcm.debug={}", parser.isSet(verboseOption))));
    }

    KDSingleApplication single;
    if (! single.isPrimaryInstance()) {
        WARN_LOG("another qcm running, triggering");
        single.sendMessageWithTimeout("hello", 5);
        exit(0);
    }

    int re { 0 };
    {
        qcm::App app { {} };
        QObject::connect(&single, &KDSingleApplication::messageReceived, app.instance(), []() {
            emit qcm::App::instance() -> instanceStarted();
        });
        app.init();

        re = gui_app.exec();
    }

    return re;
}
