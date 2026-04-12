module;
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#include "Qcm/image_provider/http.moc"
#endif

export module qcm:image_provider.http;
export import :image_provider.response;
export import :global;
export import qextra;
import ncrequest;

namespace ncm
{
class Client;
}

namespace qcm
{

class QcmImageResponse;

export class QcmAsyncImageResponse : public QcmImageResponse {
    Q_OBJECT
public:
    QcmAsyncImageResponse();
    ~QcmAsyncImageResponse() override;
    auto textureFactory() const -> QQuickTextureFactory* override;
    void cancel() override { m_wdog.cancel(); }

    auto&  wdog() { return m_wdog; }
    QImage image;

private:
    WatchDog m_wdog;
};

class QcmImageProviderInner;
export class QcmImageProvider : public QQuickAsyncImageProvider {
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
