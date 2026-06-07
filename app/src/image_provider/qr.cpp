module;
#include "qr_code/qrcodegen.hpp"
module qcm;
import :image_provider.qr;
import :app;
import :global;

namespace qcm
{
QrImageProvider::QrImageProvider(): QQuickAsyncImageProvider() {}

QQuickImageResponse* QrImageProvider::requestImageResponse(const QString& id,
                                                           const QSize&   requestedSize) {
    auto rsp = QrAsyncImageResponse::make_rc<QrAsyncImageResponse>();

    if (id.isEmpty()) {
        return rsp.get();
    }

    auto handle = QAsyncResult::runtime_handle().spawn(
        qextra::own_task([id, requestedSize, rsp]() -> task<void> {
            auto                  bs = id.toUtf8();
            up<qrcodegen::QrCode> qr_;
            try {
                qr_ = std::make_unique<qrcodegen::QrCode>(qrcodegen::QrCode::encodeBinary(
                    std::vector<u8> { bs.begin(), bs.end() }, qrcodegen::QrCode::Ecc::MEDIUM));
            } catch (const std::exception& e) {
                rsp->setError(qextra::to_qstring(rstd::format("{} ({})", e.what(), id)));
                co_return;
            }
            auto&  qr     = *qr_;
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
            co_return;
        }));
    rsp->set_task(rstd::move(handle));

    return rsp.get();
}

} // namespace qcm
