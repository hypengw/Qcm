module;
#include <deque>
#include <optional>
#include "Qcm/image_provider/http.moc.h"

module qcm;
import :image_provider.http;
import :util.path;
import :app;
import :global;
import platform;
import qextra;

using namespace qcm;

namespace
{

struct ImageParam {
    QString item_type;
    QString item_id;
    QString image_type;
};

auto parse_image_url(const QUrl& url) -> ImageParam {
    static const QRegularExpression re(R"(image://qcm/([^/]+?)/([^/]+?)/(.*))");

    const QString           input = url.toString(QUrl::FullyEncoded);
    QRegularExpressionMatch match = re.match(input);

    if (match.hasMatch()) {
        return { match.captured(1), match.captured(2), match.captured(3) };
    } else {
        return {};
    }
}

} // namespace

namespace qcm
{

QcmAsyncImageResponse::QcmAsyncImageResponse() {}
QcmAsyncImageResponse::~QcmAsyncImageResponse() { plt::malloc_trim_count(0, 10); }

QQuickTextureFactory* QcmAsyncImageResponse::textureFactory() const {
    return QQuickTextureFactory::textureFactoryForImage(image);
}

class QcmImageProviderInner : public std::enable_shared_from_this<QcmImageProviderInner>, NoCopy {
public:
    struct Job {
        rc<QcmAsyncImageResponse> rsp;
        ncrequest::Request        request;
        QSize                     requested_size;
    };

    class ActiveRequest {
    public:
        explicit ActiveRequest(rc<QcmImageProviderInner> owner): m_owner(rstd::move(owner)) {}
        ActiveRequest(const ActiveRequest&)            = delete;
        ActiveRequest& operator=(const ActiveRequest&) = delete;
        ActiveRequest(ActiveRequest&& other) noexcept: m_owner(rstd::move(other.m_owner)) {}
        ActiveRequest& operator=(ActiveRequest&&) = delete;
        ~ActiveRequest() {
            if (m_owner) m_owner->request_finished();
        }

    private:
        rc<QcmImageProviderInner> m_owner;
    };

    QcmImageProviderInner(): m_session(Global::instance()->session()) {}

    void submit(Job job) {
        auto start = std::optional<Job> {};
        {
            auto lock = std::scoped_lock { m_mutex };
            if (m_closed) {
                job.rsp->setError(QStringLiteral("image provider is closed"));
                return;
            }
            if (m_active < MaxConcurrent) {
                ++m_active;
                start.emplace(rstd::move(job));
            } else {
                m_pending.push_back(rstd::move(job));
            }
        }
        if (start) start_job(rstd::move(*start));
    }

    void shutdown() {
        auto pending = std::deque<Job> {};
        {
            auto lock = std::scoped_lock { m_mutex };
            m_closed  = true;
            pending.swap(m_pending);
        }
        for (auto& job : pending) {
            job.rsp->setError(QStringLiteral("image provider is closed"));
        }
    }

private:
    static constexpr std::size_t MaxConcurrent = 8;

    static auto error_string(auto&& error) -> QString {
        auto text = rstd::format("{}", rstd::forward<decltype(error)>(error));
        return qextra::to_qstring(text);
    }

    auto request_image(const ncrequest::Request& request, QSize requested_size)
        -> task<Result<QImage, QString>> {
        auto response = co_await m_session->get(request);
        if (response.is_err()) {
            co_return Err(error_string(rstd::move(response).unwrap_err_unchecked()));
        }

        auto body = co_await rstd::move(response).unwrap_unchecked()->bytes();
        if (body.is_err()) {
            co_return Err(error_string(rstd::move(body).unwrap_err_unchecked()));
        }

        auto bytes = rstd::move(body).unwrap_unchecked();
        auto image = QImage {};
        if (! image.loadFromData(reinterpret_cast<const uchar*>(bytes.data()),
                                 static_cast<int>(bytes.size().to_primitive()))) {
            co_return Err(QStringLiteral("image decode failed"));
        }
        if (requested_size.isValid() && ! image.isNull()) {
            image = image.scaled(requested_size,
                                 Qt::AspectRatioMode::KeepAspectRatioByExpanding,
                                 Qt::TransformationMode::SmoothTransformation);
        }
        co_return Ok(rstd::move(image));
    }

    static auto run_job(rc<QcmImageProviderInner> owner, Job job, ActiveRequest active)
        -> task<void> {
        (void)active;
        auto image = co_await owner->request_image(job.request, job.requested_size);
        if (image.is_err()) {
            job.rsp->setError(rstd::move(image).unwrap_err_unchecked());
            co_return;
        }
        job.rsp->image = rstd::move(image).unwrap_unchecked();
    }

    void start_job(Job job) {
        auto rsp    = job.rsp;
        auto self   = shared_from_this();
        auto task   = run_job(self, rstd::move(job), ActiveRequest { self });
        auto handle = QAsyncResult::runtime_handle().spawn(rstd::move(task));
        rsp->set_task(rstd::move(handle));
    }

    void request_finished() {
        auto next = std::optional<Job> {};
        {
            auto lock = std::scoped_lock { m_mutex };
            if (m_active > 0) --m_active;
            if (! m_closed && ! m_pending.empty()) {
                ++m_active;
                next.emplace(rstd::move(m_pending.front()));
                m_pending.pop_front();
            }
        }
        if (next) start_job(rstd::move(*next));
    }

    Arc<ncrequest::Session> m_session;
    std::mutex              m_mutex;
    std::deque<Job>         m_pending;
    std::size_t             m_active { 0 };
    bool                    m_closed { false };
};
} // namespace qcm

QcmImageProvider::QcmImageProvider()
    : QQuickAsyncImageProvider(), m_inner(std::make_shared<QcmImageProviderInner>()) {}
QcmImageProvider::~QcmImageProvider() { m_inner->shutdown(); }

QQuickImageResponse* QcmImageProvider::requestImageResponse(const QString& id,
                                                            const QSize&   requestedSize) {
    auto rsp = QcmAsyncImageResponse::make_rc<QcmAsyncImageResponse>();

    do {
        if (id.isEmpty()) break;

        auto req = [&]() -> Result<ncrequest::Request, QString> {
            if (id.startsWith("http")) {
                auto url      = id.toStdString();
                auto url_text = rstd::cppstd::as_str(url);
                if (url_text.is_err()) {
                    return Err(QStringLiteral("invalid image URL encoding"));
                }
                auto parsed = ncrequest::Request::from_url(
                    rstd::move(url_text).unwrap_unchecked());
                if (parsed.is_err()) {
                    auto error = rstd::move(parsed).unwrap_err_unchecked();
                    return Err(QStringLiteral("invalid image URL at byte %1")
                                   .arg(error.offset().to_primitive()));
                }
                return Ok(rstd::move(parsed).unwrap_unchecked());
            }

            auto image_url = QUrl(QStringLiteral("image://qcm/") + id);
            auto p         = parse_image_url(image_url);
            auto request = App::instance()->backend()->image(p.item_type, p.item_id, p.image_type);
            if (request.is_err()) {
                auto error = rstd::move(request).unwrap_err_unchecked();
                auto text  = rstd::format("{}", error);
                return Err(qextra::to_qstring(text));
            }
            return Ok(rstd::move(request).unwrap_unchecked());
        }();

        if (req.is_err()) {
            rsp->setError(rstd::move(req).unwrap_err_unchecked());
            return rsp.get();
        }

        auto request = rstd::move(req).unwrap_unchecked();
        request.get_opt<ncrequest::req_opt::Timeout>().transfer_timeout = rstd::i64(120'000);
        m_inner->submit(QcmImageProviderInner::Job {
            .rsp            = rsp,
            .request        = rstd::move(request),
            .requested_size = requestedSize,
        });
        return rsp.get();
    } while (false);

    return rsp.get();
}

#include "Qcm/image_provider/http.moc.cpp"
