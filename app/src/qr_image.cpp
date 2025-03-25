#include "Qcm/qr_image.h"

#include <QPointer>

#include "qr_code/qrcodegen.hpp"

#include "Qcm/app.h"
#include "qcm_interface/type.h"

namespace qcm
{
QrImageProvider::QrImageProvider(): QQuickAsyncImageProvider() {}

QQuickImageResponse* QrImageProvider::requestImageResponse(const QString& id,
                                                           const QSize&   requestedSize) {
    auto rsp = QrAsyncImageResponse::make_rc<QrAsyncImageResponse>();

    if (id.isEmpty()) {
        return rsp.get();
    }

    auto ex = qcm::pool_executor();
    asio::post(ex, [id, requestedSize, rsp]() {
        auto                  bs = id.toUtf8();
        up<qrcodegen::QrCode> qr_;
        try {
            qr_ = std::make_unique<qrcodegen::QrCode>(qrcodegen::QrCode::encodeBinary(
                std::vector<u8> { bs.begin(), bs.end() }, qrcodegen::QrCode::Ecc::MEDIUM));
        } catch (const std::exception& e) {
            rsp->set_error(fmt::format("{} ({})", e.what(), id));
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
        rsp->image = qr_img;
    });

    return rsp.get();
}

} // namespace qcm