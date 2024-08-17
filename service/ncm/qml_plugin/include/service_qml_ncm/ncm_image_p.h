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

class NcmAsyncImageResponse : public QQuickImageResponse {
    Q_OBJECT
public:
    NcmAsyncImageResponse();
    virtual ~NcmAsyncImageResponse();

    QQuickTextureFactory* textureFactory() const override;
    QString errorString() const override { return m_error; }

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

class NcmImageProviderInner;
class  NcmImageProvider : public QQuickAsyncImageProvider {
public:
    NcmImageProvider();
    ~NcmImageProvider();

    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize&   requestedSize) override;

    static request::Request makeReq(const QString& id, const QSize& requestedSize, ncm::Client&);
    static std::filesystem::path genImageCachePath(const request::Request&);

private:
    rc<NcmImageProviderInner> m_inner;
};

} // namespace qcm
