module;
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <chrono>
#include <optional>
module qcm.playback.io;
import rstd.cppstd;

namespace qcm
{
namespace
{
namespace wa = wavsen::audio;
class PlaybackIo final : public wa::MediaIo {
public:
    explicit PlaybackIo(QUrl source): source_(std::move(source)) {}
    ~PlaybackIo() override { close(); }
    auto resolve(const wa::MediaIoRequest& request)
        -> rstd::Result<wa::MediaInput, wa::MediaError> override {
        auto cancelled = [&] {
            return request.cancelled && request.cancelled(request.context);
        };
        auto fail = [&] {
            return rstd::Err(wa::MediaError { cancelled() ? wa::MediaErrorKind::Cancelled
                                                          : wa::MediaErrorKind::Open });
        };
        if (cancelled()) return fail();
        const auto raw = rstd::cppstd::as_string_view(request.uri);
        const QUrl uri(QString::fromUtf8(raw.data(), qsizetype(raw.size())), QUrl::StrictMode);
        if (! opened_) {
            auto result = post(source_, {}, request.context, request.cancelled, 30000);
            if (! result) return fail();
            root_          = source_.resolved(QUrl(result->value("url").toString()));
            session_       = result->value("session").toString();
            time_seekable_ = result->value("time_seekable").toBool();
            opened_        = true;
            touched_       = std::chrono::steady_clock::now();
            if (! local(root_)) {
                close();
                return fail();
            }
        }
        QUrl target;
        if (uri == source_ || uri == root_)
            target = root_;
        else if (! session_.isEmpty()) {
            if (local(uri) && uri.path().startsWith(prefix() + "/r/"))
                target = uri;
            else {
                auto result = post(control("resolve"),
                                   { { "uri", uri.toString(QUrl::FullyEncoded) } },
                                   request.context,
                                   request.cancelled,
                                   10000);
                if (! result) return fail();
                target = source_.resolved(QUrl(result->value("url").toString()));
                if (! local(target) || ! target.path().startsWith(prefix() + "/r/")) return fail();
            }
        } else
            return fail();
        if (cancelled()) return fail();
        auto bytes = target.toEncoded();
        auto name = rstd::cppstd::as_str(std::string_view(bytes.constData(), size_t(bytes.size())));
        if (name.is_err()) return fail();
        return rstd::Ok(
            wa::MediaInput { rstd::prelude::String::make(name.unwrap()), {}, time_seekable_ });
    }
    void keep_alive() override {
        if (session_.isEmpty() ||
            std::chrono::steady_clock::now() - touched_ < std::chrono::seconds(30))
            return;
        touched_ = std::chrono::steady_clock::now();
        (void)post(control("touch"), {}, nullptr, nullptr, 2000);
    }
    void close() noexcept override {
        if (! session_.isEmpty()) {
            (void)post(control("close"), {}, nullptr, nullptr, 2000);
            session_.clear();
        }
        opened_ = false;
    }

private:
    bool local(const QUrl& uri) const {
        return uri.isValid() && uri.scheme() == source_.scheme() && uri.host() == source_.host() &&
               uri.port() == source_.port() && uri.userInfo().isEmpty();
    }
    QString prefix() const { return "/playback/" + session_; }
    QUrl    control(const char* action) const {
        auto url = source_;
        url.setPath(prefix() + "/" + action);
        url.setQuery(QString());
        return url;
    }
    static auto post(const QUrl& url, const QJsonObject& body, void* context,
                     bool (*cancelled)(void*) noexcept, int timeout) -> std::optional<QJsonObject> {
        QNetworkAccessManager manager;
        QNetworkRequest       request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::ManualRedirectPolicy);
        auto* reply = manager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
        QEventLoop loop;
        QTimer     poll, deadline;
        deadline.setSingleShot(true);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(reply, &QNetworkReply::readyRead, &loop, [&] {
            if (reply->bytesAvailable() > 65536) reply->abort();
        });
        QObject::connect(&deadline, &QTimer::timeout, &loop, [&] {
            reply->abort();
            loop.quit();
        });
        QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
            if (cancelled && cancelled(context)) reply->abort();
        });
        poll.start(25);
        deadline.start(timeout);
        if (! reply->isFinished()) loop.exec();
        if (reply->error() != QNetworkReply::NoError || reply->bytesAvailable() > 65536 ||
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
            return std::nullopt;
        auto document = QJsonDocument::fromJson(reply->readAll());
        if (! document.isObject()) return std::nullopt;
        return document.object();
    }
    QUrl                                  source_, root_;
    QString                               session_;
    bool                                  opened_ {}, time_seekable_ {};
    std::chrono::steady_clock::time_point touched_;
};
} // namespace
auto make_playback_io(const QUrl& source) -> rc<wa::MediaIo> {
    if (source.scheme() != "http" || (source.host() != "127.0.0.1" && source.host() != "::1") ||
        ! source.path().startsWith("/playback/source/"))
        return {};
    return make_rc<PlaybackIo>(source);
}
} // namespace qcm
