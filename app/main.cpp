#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>

#include "Qcm/app.h"
#include "request/request.h"
#include "core/log.h"

#include <QtQml/QQmlExtensionPlugin>
Q_IMPORT_QML_PLUGIN(Qcm_AppPlugin)
Q_IMPORT_QML_PLUGIN(Qcm_MaterialPlugin)

#include <SingleApplication>

int main(int argc, char* argv[]) {
    // qputenv("QT_FONT_DPI", "96");
    // qputenv("QT_MEDIA_BACKEND", "ffmpeg");
    auto logger = qcm::LogManager::init();
    request::global_init();

    SingleApplication gui_app(argc, argv);
    QCoreApplication::setApplicationName("Qcm");
    QCoreApplication::setApplicationVersion(APP_VERSION);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption verboseOption("verbose");
    parser.addOption(verboseOption);
    parser.process(gui_app);

    logger->set_level(parser.isSet(verboseOption) ? qcm::LogLevel::DEBUG : qcm::LogLevel::WARN);
    int re;
    {
        QQmlApplicationEngine engine;
        engine.addImportPath(u"qrc:/"_qs);

        qcm::App* app = qcm::App::instance();
        QObject::connect(
            &gui_app, &SingleApplication::instanceStarted, app, &qcm::App::instanceStarted);

        app->init(&engine);

        re = gui_app.exec();
    }

    return re;
}
