module;

export module qcm:image_provider.qr;
export import :image_provider.response;
import qcm.core;
import qt;

namespace qcm
{

class QcmImageResponse;

export class QrAsyncImageResponse : public QcmImageResponse {
public:
    QQuickTextureFactory* textureFactory() const override {
        return QQuickTextureFactory::textureFactoryForImage(image);
    }

    QImage image;
};

export class QrImageProvider : public QQuickAsyncImageProvider {
public:
    QrImageProvider();

    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize&   requestedSize) override;
};
} // namespace qcm
