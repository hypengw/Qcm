#include <QElapsedTimer>
#include <QFileInfo>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QSGTextureProvider>
#include <QTemporaryDir>
#include <QThread>
#include <QtGui/rhi/qrhi.h>
#include <QtPlugin>
#include <memory>
#include <cstdio>

Q_IMPORT_PLUGIN(QExtraPlugin)

namespace
{
void check(bool value, const char* message) {
    if (! value) qFatal("%s", message);
}

QObject* object(QObject* owner, const char* property) {
    return owner->property(property).value<QObject*>();
}
} // namespace

int main(int argc, char** argv) {
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString& message) {
        std::fprintf(stderr, "%s\n", qPrintable(message));
    });
    QGuiApplication app(argc, argv);
    const auto      componentPath =
        argc >= 2 ? QString::fromLocal8Bit(argv[1])
                  : QFileInfo(QString::fromUtf8(__FILE__))
                        .dir()
                        .absoluteFilePath("../../app/qml/component/player/PlayingBackground.qml");
    if (qgetenv("QT_QUICK_BACKEND") == "software") {
        QQuickWindow window;
        window.resize(640, 360);
        window.show();
        QQmlEngine               engine;
        QQmlComponent            component(&engine, QUrl::fromLocalFile(componentPath));
        std::unique_ptr<QObject> owner(component.create());
        check(bool(owner), "software component failed to load");
        auto* root = qobject_cast<QQuickItem*>(owner.get());
        root->setParentItem(window.contentItem());
        root->setSize(QSizeF(640, 360));
        root->setProperty("source", QUrl("file:///missing-software-cover.png"));
        QCoreApplication::processEvents();
        check(! root->property("supported").toBool(), "software enabled GPU effects");
        check(! object(root, "current") && ! object(root, "candidate"),
              "software created GPU slots");
        qInfo("software fallback checks passed");
        return 0;
    }
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QQuickRenderControl control;
    QQuickWindow        window(&control);
    window.resize(640, 360);
    window.show();
    check(control.initialize(), "RHI initialization failed");
    auto* rhi = control.rhi();
    qInfo() << "RHI:" << rhi->backendName() << rhi->driverInfo().deviceName;
    std::unique_ptr<QRhiTexture> color(
        rhi->newTexture(QRhiTexture::RGBA8,
                        QSize(640, 360),
                        1,
                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    check(color->create(), "color texture failed");
    std::unique_ptr<QRhiRenderBuffer> depth(
        rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, QSize(640, 360)));
    check(depth->create(), "depth buffer failed");
    QRhiTextureRenderTargetDescription description(color.get());
    description.setDepthStencilBuffer(depth.get());
    std::unique_ptr<QRhiTextureRenderTarget>  target(rhi->newTextureRenderTarget(description));
    std::unique_ptr<QRhiRenderPassDescriptor> pass(target->newCompatibleRenderPassDescriptor());
    target->setRenderPassDescriptor(pass.get());
    check(target->create(), "render target failed");
    window.setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(target.get()));

    QQmlEngine               engine;
    QQmlComponent            component(&engine, QUrl::fromLocalFile(componentPath));
    std::unique_ptr<QObject> owner(component.create());
    if (! owner) qFatal("%s", qPrintable(component.errorString()));
    auto* root = qobject_cast<QQuickItem*>(owner.get());
    check(root, "background must be an Item");
    root->setParentItem(window.contentItem());
    root->setSize(QSizeF(640, 360));
    root->setProperty("fallbackColor", QColor("#262626"));
    int  largestWorkTexture = 0;
    int  observedTextures   = 0;
    auto frame              = [&](bool read = false) {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        control.polishItems();
        control.beginFrame();
        control.sync();
        control.render();
        for (auto* item : root->findChildren<QQuickItem*>()) {
            if (! item->property("textureSize").isValid() || ! item->isTextureProvider() ||
                ! item->window() || ! item->window()->isExposed())
                continue;
            auto* provider = item->textureProvider();
            if (! provider || ! provider->texture()) continue;
            const auto size    = provider->texture()->textureSize();
            largestWorkTexture = qMax(largestWorkTexture, qMax(size.width(), size.height()));
            ++observedTextures;
            check(size.width() <= 256 && size.height() <= 256, "work texture exceeded GPU budget");
        }
        QRhiReadbackResult result;
        if (read) {
            auto* batch = rhi->nextResourceUpdateBatch();
            batch->readBackTexture(QRhiReadbackDescription(color.get()), &result);
            control.commandBuffer()->resourceUpdate(batch);
        }
        control.endFrame();
        if (! read) return QImage();
        rhi->finish();
        QImage image(reinterpret_cast<const uchar*>(result.data.constData()),
                     result.pixelSize.width(),
                     result.pixelSize.height(),
                     QImage::Format_RGBA8888_Premultiplied);
        return rhi->isYUpInFramebuffer() ? image.flipped(Qt::Vertical) : image.copy();
    };
    auto wait = [&](auto predicate) {
        QElapsedTimer elapsed;
        elapsed.start();
        while (! predicate() && elapsed.elapsed() < 10000) {
            frame();
            QThread::msleep(1);
        }
        if (! predicate()) {
            qInfo() << "root" << root->property("supported") << root->property("rendering")
                    << root->isVisible() << window.isVisible() << root->property("current")
                    << root->property("candidate");
            for (auto* child : root->findChildren<QObject*>()) {
                if (child->property("stage").isValid() || child->property("status").isValid())
                    qInfo() << child->metaObject()->className() << child->property("source")
                            << child->property("stage") << child->property("status")
                            << child->property("active");
            }
        }
        check(predicate(), "background did not reach expected state");
    };
    QTemporaryDir images;
    check(images.isValid(), "temporary directory failed");
    auto makeImage = [&](const char* name, QColor left, QColor right) {
        QImage image(256, 256, QImage::Format_RGB32);
        image.fill(left);
        QPainter painter(&image);
        painter.fillRect(128, 0, 128, 256, right);
        painter.end();
        auto path = images.filePath(QLatin1String(name) + ".png");
        check(image.save(path), "save input failed");
        return QUrl::fromLocalFile(path);
    };
    const auto red     = makeImage("red", Qt::red, Qt::red);
    const auto green   = makeImage("green", Qt::green, Qt::green);
    const auto blue    = makeImage("blue", Qt::blue, Qt::blue);
    const auto pattern = makeImage("pattern", Qt::red, Qt::blue);
    QImage     structured(256, 256, QImage::Format_RGB32);
    structured.fill(QColor("#173552"));
    {
        QPainter painter(&structured);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(10, 15, 50, 225, QColor("#d94731"));
        painter.setPen(QPen(QColor("#fff5bc"), 5));
        painter.setBrush(QColor("#e5b491"));
        painter.drawEllipse(QRectF(100, 45, 75, 90));
        painter.setBrush(QColor("#32958e"));
        painter.drawRoundedRect(QRectF(85, 143, 115, 110), 35, 35);
        painter.setPen(Qt::white);
        painter.setFont(QFont("sans-serif", 24, QFont::Bold));
        painter.drawText(QRect(0, 0, 256, 40), Qt::AlignCenter, "ALBUM");
        painter.setPen(QPen(Qt::white, 2));
        for (int x = 210; x < 250; x += 8) painter.drawLine(x, 40, x, 230);
    }
    const auto structuredPath = images.filePath("structured.png");
    check(structured.save(structuredPath), "save structured input failed");
    const auto structuredSource = QUrl::fromLocalFile(structuredPath);
    auto       settled          = [&](const QUrl& source) {
        auto* current = object(root, "current");
        return current && current->property("source").toUrl() == source &&
               current->property("frozen").toBool() && ! object(root, "candidate");
    };
    root->setProperty("source", red);
    wait([&] {
        return settled(red);
    });
    auto image = frame(true);
    check(image.pixelColor(320, 180).red() > 240, "first snapshot was empty or stale");
    check(! object(root, "current")->findChild<QObject*>("coverInput"),
          "frozen background retained image pipeline");
    root->setProperty("source", green);
    wait([&] {
        return object(root, "candidate") && object(root, "candidate")->property("frozen").toBool();
    });
    root->setProperty("source", blue);
    wait([&] {
        auto* next = object(root, "candidate");
        return next && next->property("source").toUrl() == green &&
               next->property("opacity").toReal() >= 0.25;
    });
    check(root->findChildren<QObject*>("coverInput").isEmpty(),
          "crossfade retained a blur input pipeline");
    auto blend = frame(true).pixelColor(320, 180);
    check(blend.red() + blend.green() > 230 && blend.blue() < 5,
          "crossfade exposed the fallback between covers");
    wait([&] {
        return settled(blue);
    });
    image = frame(true);
    check(image.pixelColor(320, 180).blue() > 240, "rapid replacement displayed stale source");
    auto* stable = object(root, "current");
    root->setProperty("source", blue);
    frame();
    check(object(root, "current") == stable && ! object(root, "candidate"),
          "same source regenerated a snapshot");
    root->setProperty("source", structuredSource);
    wait([&] {
        return settled(structuredSource);
    });
    image               = frame(true);
    int contourContrast = 0;
    for (int y = 0; y < image.height() - 8; y += 4) {
        for (int x = 0; x < image.width() - 8; x += 4) {
            const auto center = image.pixelColor(x, y);
            for (const auto neighbor : { image.pixelColor(x + 8, y), image.pixelColor(x, y + 8) }) {
                contourContrast = qMax(contourContrast, qAbs(center.red() - neighbor.red()));
                contourContrast = qMax(contourContrast, qAbs(center.green() - neighbor.green()));
                contourContrast = qMax(contourContrast, qAbs(center.blue() - neighbor.blue()));
            }
        }
    }
    qInfo() << "structured cover contrast over 8 pixels:" << contourContrast;
    check(contourContrast <= 8, "blur retained high-contrast cover contours");
    if (argc >= 3) {
        const auto output = QString::fromLocal8Bit(argv[2]);
        check(image.save(output + ".structured.png"), "save structured output failed");
        check(structured.save(output + ".source.png"), "save structured source failed");
    }
    root->setProperty("source", pattern);
    wait([&] {
        return settled(pattern);
    });
    image       = frame(true);
    auto middle = image.pixelColor(320, 180);
    check(middle.red() > 60 && middle.blue() > 60, "cover boundary was not blurred");
    const auto left  = image.pixelColor(20, 180);
    const auto right = image.pixelColor(620, 180);
    check(left.red() > left.blue() + 40, "cover crop lost left orientation");
    check(right.blue() > right.red() + 40, "cover crop lost right orientation");
    if (argc >= 3) check(image.save(QString::fromLocal8Bit(argv[2])), "save evidence failed");
    auto* retained = object(root, "current");
    root->setSize(QSizeF(320, 600));
    frame();
    check(object(root, "current") == retained, "resize regenerated the background");
    root->setSize(QSizeF(3840, 2160));
    root->setProperty("source", red);
    wait([&] {
        return settled(red);
    });
    check(object(root, "current")->property("decodeSide").toInt() == 512,
          "4K decode exceeded budget");
    root->setSize(QSizeF(640, 360));
    root->setVisible(false);
    frame();
    check(! object(root, "current") && ! object(root, "candidate"),
          "hidden page retained background");
    root->setVisible(true);
    wait([&] {
        return settled(red);
    });
    root->setProperty("active", false);
    frame();
    check(! object(root, "current") && ! object(root, "candidate"),
          "inactive background retained work");
    root->setProperty("source", green);
    root->setProperty("active", true);
    wait([&] {
        return settled(green);
    });
    check(frame(true).pixelColor(320, 180).green() > 240, "resume did not load latest source");
    window.hide();
    frame();
    check(! object(root, "current") && ! object(root, "candidate"),
          "hidden window retained background");
    window.show();
    wait([&] {
        return settled(green);
    });
    root->setProperty("source", QUrl::fromLocalFile(images.filePath("missing.png")));
    wait([&] {
        return ! object(root, "current") && ! object(root, "candidate");
    });
    check(frame(true).pixelColor(320, 180) == QColor("#262626"), "error retained old cover");
    root->setProperty("source", red);
    root->setProperty("source", blue);
    wait([&] {
        return settled(blue);
    });
    control.invalidate();
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    check(! object(root, "current") && ! object(root, "candidate"),
          "invalidated scene graph retained snapshots");
    check(control.initialize(), "RHI reinitialization failed");
    check(control.rhi() == rhi, "test render target lost its RHI");
    wait([&] {
        return settled(blue);
    });
    check(frame(true).pixelColor(320, 180).blue() > 240,
          "scene graph recovery lost the current cover");
    root->setProperty("source", QUrl());
    frame();
    check(! object(root, "current") && ! object(root, "candidate"), "empty source retained cover");
    check(observedTextures > 0 && largestWorkTexture == 256, "no GPU snapshot textures observed");
    qInfo() << "GPU work textures: max side" << largestWorkTexture << "DPR"
            << root->property("pixelRatio");
    QQmlComponent maskComponent(
        &engine,
        QUrl::fromLocalFile(QFileInfo(componentPath).dir().filePath("PlayingBackgroundMask.qml")));
    std::unique_ptr<QObject> maskOwner(maskComponent.create());
    if (! maskOwner) qFatal("%s", qPrintable(maskComponent.errorString()));
    auto* mask = qobject_cast<QQuickItem*>(maskOwner.get());
    check(mask, "mask must be an Item");
    mask->setParentItem(root);
    mask->setZ(3);
    mask->setTransformOrigin(QQuickItem::TopLeft);
    root->setProperty("fallbackColor", QColor(Qt::white));
    for (const auto height : { 360, 800 }) {
        const auto scale = 360.0 / height;
        mask->setSize(QSizeF(640 / scale, height));
        mask->setScale(scale);
        image = frame(true);
        check(qAbs(image.pixelColor(320, 0).red() - 132) <= 3,
              "mask top opacity differs from the traditional player");
        check(qAbs(image.pixelColor(320, 359).red() - 67) <= 3,
              "mask bottom opacity differs from the traditional player");
        check(image.pixelColor(320, 180).red() > image.pixelColor(320, 0).red() + 10,
              "mask center must be lighter than its top");
        if (height == 800) {
            check(qAbs(image.pixelColor(320, 100).red() - 148) <= 2 &&
                      qAbs(image.pixelColor(320, 210).red() - 148) <= 2,
                  "mask did not stretch its center independently of its borders");
        }
    }
    mask->setSize(QSizeF(640, 360));
    mask->setScale(1);
    root->setProperty("source", structuredSource);
    wait([&] {
        return settled(structuredSource);
    });
    if (argc >= 3)
        check(frame(true).save(QString::fromLocal8Bit(argv[2]) + ".masked.png"),
              "save masked background failed");
    root->setProperty("source", QUrl());
    frame();
    const QSize profileSize(8, 1362);
    color->setPixelSize(profileSize);
    depth->setPixelSize(profileSize);
    check(color->create() && depth->create() && target->create(), "profile render target failed");
    window.resize(profileSize);
    window.setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(target.get()));
    root->setSize(profileSize);
    mask->setSize(QSizeF(8.0 / 3, 454));
    mask->setScale(3);
    image = frame(true);
    // Alpha runs from the APK's original 10x1362 mask, sampled before fitting.
    constexpr int alphaRuns[][2] = {
        { 0, 122 },    { 1, 123 },    { 5, 122 },    { 8, 123 },    { 9, 122 },    { 12, 123 },
        { 26, 122 },   { 27, 123 },   { 42, 122 },   { 43, 123 },   { 54, 122 },   { 58, 123 },
        { 61, 122 },   { 63, 123 },   { 79, 122 },   { 80, 123 },   { 87, 122 },   { 89, 123 },
        { 94, 122 },   { 96, 123 },   { 104, 122 },  { 107, 123 },  { 108, 122 },  { 110, 123 },
        { 114, 122 },  { 161, 121 },  { 162, 122 },  { 167, 121 },  { 187, 120 },  { 188, 121 },
        { 189, 120 },  { 222, 119 },  { 246, 118 },  { 256, 117 },  { 257, 118 },  { 263, 117 },
        { 266, 116 },  { 269, 117 },  { 272, 116 },  { 292, 115 },  { 295, 116 },  { 299, 115 },
        { 302, 114 },  { 323, 113 },  { 348, 112 },  { 368, 110 },  { 370, 112 },  { 380, 110 },
        { 406, 109 },  { 440, 108 },  { 461, 107 },  { 495, 106 },  { 746, 107 },  { 780, 108 },
        { 801, 109 },  { 835, 110 },  { 862, 112 },  { 885, 113 },  { 904, 114 },  { 915, 115 },
        { 919, 116 },  { 939, 117 },  { 943, 118 },  { 949, 119 },  { 959, 120 },  { 975, 121 },
        { 979, 122 },  { 992, 123 },  { 994, 124 },  { 1007, 125 }, { 1011, 126 }, { 1017, 127 },
        { 1023, 128 }, { 1029, 129 }, { 1036, 130 }, { 1042, 131 }, { 1052, 132 }, { 1055, 133 },
        { 1065, 134 }, { 1074, 136 }, { 1081, 137 }, { 1087, 138 }, { 1090, 139 }, { 1094, 140 },
        { 1100, 141 }, { 1106, 142 }, { 1114, 143 }, { 1120, 144 }, { 1124, 145 }, { 1127, 146 },
        { 1133, 147 }, { 1137, 148 }, { 1140, 149 }, { 1148, 150 }, { 1154, 151 }, { 1157, 152 },
        { 1162, 153 }, { 1165, 154 }, { 1176, 155 }, { 1179, 156 }, { 1180, 157 }, { 1188, 158 },
        { 1196, 159 }, { 1199, 160 }, { 1203, 161 }, { 1208, 162 }, { 1211, 163 }, { 1215, 164 },
        { 1224, 165 }, { 1228, 166 }, { 1236, 167 }, { 1241, 168 }, { 1245, 169 }, { 1250, 170 },
        { 1254, 171 }, { 1255, 172 }, { 1264, 173 }, { 1274, 174 }, { 1275, 175 }, { 1284, 176 },
        { 1295, 177 }, { 1296, 178 }, { 1301, 179 }, { 1312, 181 }, { 1319, 182 }, { 1332, 183 },
        { 1338, 184 }, { 1345, 185 }, { 1353, 187 }, { 1360, 188 }, { 1361, 184 }
    };
    int run           = 0;
    int maxAlphaError = 0;
    for (int y = 0; y < image.height(); ++y) {
        if (run + 1 < int(sizeof(alphaRuns) / sizeof(alphaRuns[0])) && y == alphaRuns[run + 1][0])
            ++run;
        const int actual = 255 - image.pixelColor(4, y).red();
        maxAlphaError    = qMax(maxAlphaError, qAbs(actual - alphaRuns[run][1]));
    }
    qInfo() << "mask rendered alpha error across 1362 source rows:" << maxAlphaError;
    check(maxAlphaError <= 1, "mask rendered alpha differs from the original resource");
    maskOwner.reset();
    qInfo("traditional player mask checks passed");
    qInfo("playing background checks passed");
    owner.reset();
    control.invalidate();
    return 0;
}
