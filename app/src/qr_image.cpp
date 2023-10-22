#include "Qcm/qr_image.h"

#include <QPointer>

#include "qr_code/qrcodegen.hpp"

#include "Qcm/app.h"
#include "Qcm/type.h"
#include "core/expected_helper.h"

using namespace qcm;

QrImageProvider::QrImageProvider()
    : QQuickAsyncImageProvider(), m_ex(App::instance()->get_pool_executor()) {}

QQuickImageResponse* QrImageProvider::requestImageResponse(const QString& id,
                                                           const QSize&   requestedSize) {
    QrAsyncImageResponse* rsp = new QrAsyncImageResponse();

    if (id.isEmpty()) {
        rsp->finished();
        return rsp;
    }

    auto rsp_guard = QPointer(rsp);
    auto handle    = [rsp_guard](nstd::expected<QImage, QString> res) {
        if (res) {
            QMetaObject::invokeMethod(
                rsp_guard, "handle", Qt::QueuedConnection, Q_ARG(QImage, res.value()));
        } else {
            QMetaObject::invokeMethod(
                rsp_guard, "handle_error", Qt::QueuedConnection, Q_ARG(QString, res.error()));
        }
    };

    asio::post(m_ex, [id, requestedSize, handle]() {
        auto                  bs = id.toUtf8();
        up<qrcodegen::QrCode> qr_;
        try {
            qr_ = std::make_unique<qrcodegen::QrCode>(qrcodegen::QrCode::encodeBinary(
                std::vector<u8> { bs.begin(), bs.end() }, qrcodegen::QrCode::Ecc::MEDIUM));
        } catch (const std::exception& e) {
            handle(nstd::unexpected(convert_from<QString>(fmt::format("{} ({})", e.what(), id))));
            return;
        }
        auto& qr = *qr_;
        // 创建二维码画布
        QImage qr_img = QImage(qr.getSize(), qr.getSize(), QImage::Format_RGB888);

        for (int y = 0; y < qr.getSize(); y++) {
            for (int x = 0; x < qr.getSize(); x++) {
                if (qr.getModule(x, y) == 0)
                    qr_img.setPixel(x, y, qRgb(255, 255, 255));
                else
                    qr_img.setPixel(x, y, qRgb(0, 0, 0));
            }
        }
        if (requestedSize.isValid()) qr_img = qr_img.scaled(requestedSize);
        handle(qr_img);
    });

    return rsp;
}
