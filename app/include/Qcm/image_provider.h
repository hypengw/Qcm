#pragma once

#include <filesystem>

#include <QQuickImageProvider>
#include <QQuickAsyncImageProvider>

#include "core/asio/helper.h"
#include "core/asio/watch_dog.h"
#include "Qcm/image_response.h"

import ncrequest;

namespace ncm
{
class Client;
}

namespace qcm
{

class QcmAsyncImageResponse : public QcmImageResponse {
    Q_OBJECT
public:
    QcmAsyncImageResponse();
    ~QcmAsyncImageResponse() override;
    auto textureFactory() const -> QQuickTextureFactory* override;
    void cancel() override { m_wdog.cancel(); }

    auto&  wdog() { return m_wdog; }
    QImage image;

private:
    helper::WatchDog m_wdog;
};

class QcmImageProviderInner;
class QcmImageProvider : public QQuickAsyncImageProvider {
public:
    QcmImageProvider();
    ~QcmImageProvider();

    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize&   requestedSize) override;

    static ncrequest::Request makeReq(const QString& id, const QSize& requestedSize, ncm::Client&);

private:
    rc<QcmImageProviderInner> m_inner;
};

} // namespace qcm