#pragma once

#include <QQuickImageProvider>
#include <QQuickAsyncImageProvider>

#include "asio_helper/helper.h"
#include "asio_helper/watch_dog.h"
#include "ncm/client.h"

namespace qcm
{

class QrAsyncImageResponse : public QQuickImageResponse {
    Q_OBJECT
public:
    QrAsyncImageResponse() {}
    virtual ~QrAsyncImageResponse() {}

    QQuickTextureFactory* textureFactory() const override {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    QString errorString() const override { return m_error; }

public slots:
    void handle(QImage img) {
        m_image = img;
        emit finished();
    }
    void handle_error(QString error) {
        m_error = error;
        emit finished();
    }

private:
    QImage  m_image;
    QString m_error;
};

class QrImageProvider : public QQuickAsyncImageProvider {
public:
    QrImageProvider();

    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize&   requestedSize) override;

private:
    asio::any_io_executor m_ex;
};
} // namespace qcm
