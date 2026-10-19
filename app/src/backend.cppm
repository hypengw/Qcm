module;
#include "core/log.h"
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/backend.moc"
#endif
export module qcm:backend;
export import :msg;
export import :status.process;
export import :util.mem;
import qextra;
import ncrequest;

using rstd::boxed::Box;
using rstd::sync::Arc;
using rstd::sync::atomic::Atomic;
using namespace qextra::prelude;

export namespace qcm
{

namespace detail
{
class BackendTransport;
} // namespace detail
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl resourceBaseUrl READ resource_base_url NOTIFY resourceStateChanged FINAL)

    friend class detail::BackendTransport;

public:
    Backend();
    ~Backend();

    auto start(QStringView exe, QStringView data_dir, QStringView cache_dir) -> bool;

    void send_immediate(msg::QcmMessage&& msg);

    auto send(msg::QcmMessage&& msg) -> task<Result<msg::QcmMessage, msg::Error>>;

    auto image_url(model::ItemId id, enums::ImageType image_type) const -> QUrl;
    auto resource_base_url() const -> QUrl;
    auto cache_directory() const -> QString;
    auto audio_url(model::ItemId id) -> QUrl;
    auto playback_url(model::ItemId id) -> QUrl;

    template<typename Req>
        requires msg::ReqMsgCP<Req>
    auto send(Req&& req) -> task<Result<typename msg::MsgTraits<Req>::Rsp, msg::Error>> {
        using Rsp = typename msg::MsgTraits<Req>::Rsp;
        auto msg  = msg::QcmMessage();
        msg::MsgTraits<Req>::set(msg, rstd::forward<Req>(req));
        co_return (co_await send(rstd::move(msg))).and_then([](auto m) -> Result<Rsp, msg::Error> {
            if (m.hasRsp()) {
                if (m.rsp().code() != msg::ErrorCodeGadget::ErrorCode::ERROR_CODE_OK) {
                    return Err(msg::Error { .code    = (i32)m.rsp().code(),
                                            .message = m.rsp().message().toStdString() });
                }
            }
            return msg::get_rsp<Req>(m).ok_or(msg::Error {});
        });
    }

    Q_SIGNAL void started(i32 port);
    Q_SIGNAL void connected(i32 port);
    Q_SIGNAL void error(QString);
    Q_SIGNAL void disconnected();
    Q_SIGNAL void resourceStateChanged();

    Q_SLOT void on_retry();

private:
    Q_SLOT void on_error(QString);
    Q_SLOT void on_started(i32 port);
    Q_SLOT void on_connected(i32 port);

    auto base() const -> std::string;
    auto serial() -> i32;

    Box<QThread>              m_thread;
    detail::BackendTransport* m_transport;

    Atomic<i32> m_serial;
    i32         m_port;
    bool        m_resource_ready { false };
    QString     m_exe;
    QString     m_data_dir;
    QString     m_cache_dir;
};
} // namespace qcm
