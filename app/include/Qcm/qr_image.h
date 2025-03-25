#pragma once

#include <QQuickImageProvider>
#include <QQuickAsyncImageProvider>

#include "core/asio/helper.h"
#include "Qcm/image_response.h"

namespace qcm
{

class QrAsyncImageResponse : public QcmImageResponse {
public:
    QQuickTextureFactory* textureFactory() const override {
        return QQuickTextureFactory::textureFactoryForImage(image);
    }

    QImage image;
};

class QrImageProvider : public QQuickAsyncImageProvider {
public:
    QrImageProvider();

    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize&   requestedSize) override;
};
} // namespace qcm
