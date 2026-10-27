#include <QGuiApplication>
#include <QImage>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <cstdio>
import qcm;
import qextra;

int main(int argc, char** argv) {
    if (argc != 4) {
        std::fprintf(stderr, "usage: comments-test BACKEND CHECK_QML CAPTURE_BASE\n");
        return 1;
    }
    QGuiApplication gui(argc, argv);
    QCoreApplication::setApplicationName("QcmCommentsTest");
    qcm::App          app(QString::fromLocal8Bit(argv[1]), {});
    QAsyncResult      jobs;
    QPointer<QObject> checks;
    QTimer            capture;
    capture.setInterval(50);
    QObject::connect(&capture, &QTimer::timeout, &gui, [&] {
        if (! checks) return;
        const auto path = checks->property("captureName").toString();
        if (path.isEmpty()) return;
        const auto window = qobject_cast<QQuickWindow*>(app.engine()->rootObjects().front());
        if (! window || ! window->grabWindow().save(path)) {
            std::fprintf(stderr, "cannot capture comments window\n");
            gui.exit(1);
            return;
        }
        checks->setProperty("captureName", QString());
        checks->setProperty("captured", true);
    });
    QTimer probe;
    probe.setInterval(50);
    bool pending = false;
    QObject::connect(&probe, &QTimer::timeout, &gui, [&] {
        if (pending) return;
        pending = true;
        jobs.spawn([&]() -> qextra::prelude::task<void> {
            qcm::msg::GetSongsByIdReq request;
            request.setIds({ 1 });
            auto songs = co_await app.backend()->send(std::move(request));
            co_await QAsyncResult::qexecutor();
            pending = false;
            if (! songs || songs->ids().isEmpty()) co_return;
            probe.stop();
            auto engine = app.engine();
            engine->rootContext()->setContextProperty("testWindow", engine->rootObjects().front());
            engine->rootContext()->setContextProperty(
                "testItemId",
                QVariant::fromValue(qcm::model::ItemId(qcm::enums::ItemType::ItemSong, 1)));
            engine->rootContext()->setContextProperty("captureBase",
                                                      QString::fromLocal8Bit(argv[3]));
            engine->load(QUrl::fromLocalFile(QString::fromLocal8Bit(argv[2])));
            checks = engine->rootObjects().back();
            capture.start();
        });
    });
    bool started = false;
    QObject::connect(app.backend(), &qcm::Backend::connected, &gui, [&] {
        if (started) return;
        started = true;
        jobs.spawn([&]() -> qextra::prelude::task<void> {
            qcm::msg::CreateTmpProviderReq create;
            create.setTypeName("comments-test");
            auto tmp = co_await app.backend()->send(std::move(create));
            if (! tmp) {
                std::fprintf(stderr, "cannot create test provider\n");
                co_await QAsyncResult::qexecutor();
                gui.exit(1);
                co_return;
            }
            qcm::msg::AddProviderReq add;
            add.setTmpProvider(tmp->key());
            add.setName("Comments test");
            auto added = co_await app.backend()->send(std::move(add));
            if (! added) {
                co_await QAsyncResult::qexecutor();
                gui.exit(1);
                co_return;
            }
            qcm::msg::SyncReq sync;
            sync.setProviderId(1);
            auto result = co_await app.backend()->send(std::move(sync));
            if (! result) {
                co_await QAsyncResult::qexecutor();
                gui.exit(1);
            }
            co_await QAsyncResult::qexecutor();
            probe.start();
        });
    });
    app.init();
    QTimer::singleShot(30000, &gui, [&] {
        std::fprintf(stderr, "comments ui timeout\n");
        gui.exit(1);
    });
    return gui.exec();
}
