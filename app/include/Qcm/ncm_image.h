#include <QQuickImageProvider>
#include <QQuickAsyncImageProvider>

#include "asio_helper/helper.h"
#include "asio_helper/watch_dog.h"
#include "ncm/client.h"

namespace qcm
{

class NcmAsyncImageResponse : public QQuickImageResponse {
    Q_OBJECT
public:
    NcmAsyncImageResponse() {}
    virtual ~NcmAsyncImageResponse() {}

    QQuickTextureFactory* textureFactory() const override {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

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
    QImage           m_image;
    QString          m_error;
    helper::WatchDog m_wdog;
};

class NcmImageProvider : public QQuickAsyncImageProvider {
public:
    NcmImageProvider();

    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize&   requestedSize) override;

private:
    asio::any_io_executor m_ex;
    ncm::Client           m_cli;
};

} // namespace qcm
