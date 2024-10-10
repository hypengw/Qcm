#pragma once

#include <filesystem>

#include <QQuickImageProvider>
#include <QQuickAsyncImageProvider>

#include "asio_helper/helper.h"
#include "asio_helper/watch_dog.h"

#include "request/request.h"

namespace ncm
{
class Client;
}

namespace qcm
{

class QcmAsyncImageResponse : public QQuickImageResponse {
    Q_OBJECT
public:
    QcmAsyncImageResponse();
    virtual ~QcmAsyncImageResponse();

    QQuickTextureFactory* textureFactory() const override;
    QString               errorString() const override { return m_error; }

    helper::WatchDog& wdog() { return m_wdog; }

public slots:
    void handle(QImage img) {
        m_image = img;
        emit finished();
    }
    void handle_error(QString error) {
        m_error = error;
        emit finished();
    }
    void cancel() override {
        m_wdog.cancel();
        emit finished();
    }

private:
    QImage           m_image_;
    QImage           m_image;
    QString          m_error;
    helper::WatchDog m_wdog;
};

class QcmImageProviderInner;
class QcmImageProvider : public QQuickAsyncImageProvider {
public:
    QcmImageProvider();
    ~QcmImageProvider();

    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize&   requestedSize) override;

    static request::Request makeReq(const QString& id, const QSize& requestedSize, ncm::Client&);

private:
    rc<QcmImageProviderInner> m_inner;
};

} // namespace qcm