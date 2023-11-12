#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QSurfaceFormat>
#include <QLoggingCategory>

#include "Qcm/app.h"
#include "request/request.h"
#include "core/log.h"

#include <QtQml/QQmlExtensionPlugin>
Q_IMPORT_QML_PLUGIN(Qcm_AppPlugin)
Q_IMPORT_QML_PLUGIN(Qcm_MaterialPlugin)

#include <kdsingleapplication.h>

int main(int argc, char* argv[]) {
    qputenv("QT_FONT_DPI", "96");
    auto logger = qcm::LogManager::init();
    request::global_init();

    QGuiApplication gui_app(argc, argv);

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VERSION);

    QCommandLineParser parser;
    {
        parser.addHelpOption();
        parser.addVersionOption();
        QCommandLineOption verboseOption("verbose");
        parser.addOption(verboseOption);
        parser.process(gui_app);

        logger->set_level(parser.isSet(verboseOption) ? qcm::LogLevel::DEBUG : qcm::LogLevel::WARN);
        QLoggingCategory::setFilterRules(
            QString::fromStdString(fmt::format("qcm.debug={}", parser.isSet(verboseOption))));
    }

    KDSingleApplication single;
    if (! single.isPrimaryInstance()) {
        WARN_LOG("another qcm running, triggering");
        single.sendMessageWithTimeout("hello", 5);
        exit(0);
    }

    int re;
    {
        QSurfaceFormat format;
        format.setSamples(4);
        // QSurfaceFormat::setDefaultFormat(format);

        QQmlApplicationEngine engine;
        engine.addImportPath(u"qrc:/"_qs);

        qcm::App* app = qcm::App::instance();
        QObject::connect(&single, &KDSingleApplication::messageReceived, app, [app]() {
            emit app->instanceStarted();
        });

        app->init(&engine);

        re = gui_app.exec();
    }

    return re;
}
