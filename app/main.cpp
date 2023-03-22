#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>

#include "Qcm/app.h"
#include "request/request.h"
#include "core/log.h"

int main(int argc, char* argv[]) {
    // qputenv("QT_FONT_DPI", "96");
    // qputenv("QT_MEDIA_BACKEND", "ffmpeg");
    auto logger = qcm::LogManager::init();
    request::global_init();

    QGuiApplication gui_app(argc, argv);
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

        qcm::App* app = qcm::App::instance();
        app->init(&engine);

        re = gui_app.exec();
    }

    return re;
}
