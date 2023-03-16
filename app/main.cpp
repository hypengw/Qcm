#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "Qcm/app.h"
#include "request/request.h"

int main(int argc, char* argv[]) {
    qputenv("QT_SCALE_FACTOR", "1");
    qputenv("QT_FONT_DPI", "96");
    // qputenv("QT_MEDIA_BACKEND", "ffmpeg");

    request::global_init();

    QGuiApplication gui_app(argc, argv);

    int re;
    {
        QQmlApplicationEngine engine;

        qcm::App* app = qcm::App::instance();
        app->init(&engine);

        re = gui_app.exec();
    }

    return re;
}
