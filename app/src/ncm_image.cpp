#include "Qcm/ncm_image.h"

#include <filesystem>
#include <cstdio>
#include <fstream>

#include <ctre.hpp>
#include <QtCore/QPointer>

#include "Qcm/app.h"
#include "Qcm/type.h"
#include "Qcm/path.h"
#include "Qcm/cache_sql.h"

#include "core/expected_helper.h"
#include "request/response.h"
#include "asio_helper/sync_file.h"
#include "crypto/crypto.h"
#include "ncm/client.h"

using namespace qcm;

namespace
{
constexpr int DEF_SIZE { 240 };

inline QSize get_down_size(const QSize& req) {
    const int def_size = DEF_SIZE * qApp->devicePixelRatio();

    if (req.width() <= def_size) {
        double rate = req.height() / (double)req.width();
        if (rate < 1.0) {
            return { def_size, (int)(def_size * rate) };
        } else {
            return { (int)(def_size / rate), DEF_SIZE };
        }
    }
    return req;
}

inline std::string gen_file_name(const request::Url& url) {
    return crypto::digest(crypto::md5(), To<std::vector<byte>>::from(url.path + url.query))
        .map(crypto::hex::encode_up)
        .map(To<std::string>::from<crypto::bytes_view>)
        .map_error([](auto) {
            _assert_(false);
        })
        .value();
}

void header_record_db(const request::Header& h, CacheSql::Item& db_it) {
    static constexpr auto DigitPattern = ctll::fixed_string { "\\d*" };
    if (h.contains("content-type")) db_it.content_type = h.at("content-type");
    if (h.contains("content-length")) {
        if (auto whole = ctre::starts_with<DigitPattern>(h.at("content-length")); whole) {
            db_it.content_length = whole.to_number();
        }
    }
}

} // namespace

namespace qcm
{

class NcmImageProviderInner : std::enable_shared_from_this<NcmImageProviderInner>, NoCopy {
public:
    using executor_type = asio::thread_pool::executor_type;

    executor_type& get_executor() { return m_ex; }
    auto&          get_client() { return m_cli; }

    NcmImageProviderInner()
        : m_ex(App::instance()->get_pool_executor()),
          m_cli(App::instance()->ncm_client()),
          m_cache_sql(App::instance()->get_cache_sql()) {}

    asio::awaitable<request::Header> dl_image(const request::Request& req,
                                              std::filesystem::path   p) {
        helper::SyncFile file { std::fstream(p, std::ios::out | std::ios::binary) };
        file.handle().exceptions(std::ios_base::failbit | std::ios_base::badbit);

        auto rsp_http = co_await m_cli.rsp(req);
        co_await rsp_http->read_to_stream(file);

        file.handle().close();
        co_return rsp_http->header();
    }

    asio::awaitable<void> cache_new_image(const request::Request& req, std::string_view key,
                                          std::filesystem::path cache_file, QSize req_size) {
        CacheSql::Item db_it;
        db_it.key = key;

        auto file_dl = cache_file;
        file_dl.replace_extension(fmt::format("dl{}x{}", req_size.width(), req_size.height()));

        auto header = co_await dl_image(req, file_dl);
        std::filesystem::rename(file_dl, cache_file);
        header_record_db(header, db_it);
        co_await m_cache_sql->insert(db_it);
    }

    asio::awaitable<QImage> request_image(const request::Request& req,
                                          std::filesystem::path cache_path, QSize req_size) {
        std::string key = cache_path.filename().native();
        asio::co_spawn(m_cache_sql->get_executor(), m_cache_sql->get(key), asio::detached);
        if (! std::filesystem::exists(cache_path)) {
            co_await cache_new_image(req, key, cache_path, req_size);
        }
        auto img = QImage(cache_path.c_str());
        if (req_size.isValid()) {
            img = img.scaled(req_size);
        }
        co_return img;
    }

    asio::awaitable<void> handle_request(QPointer<NcmAsyncImageResponse> rsp_guard,
                                         request::Request req, std::filesystem::path cache_path,
                                         QSize req_size) {
        auto img = co_await request_image(req, cache_path, req_size);
        NcmImageProviderInner::handle_res(rsp_guard, img);
        co_return;
    }

    static void handle_res(QPointer<NcmAsyncImageResponse> rsp_guard,
                           nstd::expected<QImage, QString> res) {
        if (res.has_value()) {
            QMetaObject::invokeMethod(
                rsp_guard, "handle", Qt::QueuedConnection, Q_ARG(QImage, res.value()));
        } else {
            QMetaObject::invokeMethod(
                rsp_guard, "handle_error", Qt::QueuedConnection, Q_ARG(QString, res.error()));
        }
    };

private:
    executor_type m_ex;
    ncm::Client   m_cli;
    rc<CacheSql>  m_cache_sql;
};
} // namespace qcm

request::Request NcmImageProvider::makeReq(const QString& id, const QSize& requestedSize,
                                           ncm::Client& cli) {
    auto               down_size = get_down_size(requestedSize);
    request::UrlParams query;
    query.set_param("param", fmt::format("{}y{}", down_size.width(), down_size.height()));
    auto req = cli.make_req<ncm::api::CryptoType::NONE>(id.toStdString(), query);
    return req;
}
std::filesystem::path NcmImageProvider::genImageCachePath(const request::Request& req) {
    auto path = cache_path() / "cache";
    std::filesystem::create_directories(path);
    return path / gen_file_name(req.url_info());
}

NcmImageProvider::NcmImageProvider()
    : QQuickAsyncImageProvider(), m_inner(std::make_shared<NcmImageProviderInner>()) {}
NcmImageProvider::~NcmImageProvider() {}

QQuickImageResponse* NcmImageProvider::requestImageResponse(const QString& id,
                                                            const QSize&   requestedSize) {
    NcmAsyncImageResponse* rsp = new NcmAsyncImageResponse();

    if (id.isEmpty()) {
        rsp->finished();
        return rsp;
    }

    auto ex = asio::make_strand(m_inner->get_executor());

    auto                  rsp_guard = QPointer(rsp);
    request::Request      req = NcmImageProvider::makeReq(id, requestedSize, m_inner->get_client());
    std::filesystem::path file_path = NcmImageProvider::genImageCachePath(req);

    asio::co_spawn(
        ex,
        rsp->wdog().watch(
            ex,
            [rsp_guard, requestedSize, req, file_path, inner = m_inner]() -> asio::awaitable<void> {
                co_await inner->handle_request(rsp_guard, req, file_path, requestedSize);
                co_return;
            }),
        [rsp_guard, file_path, id](std::exception_ptr p) {
            if (p) {
                try {
                    if (std::filesystem::exists(file_path)) {
                        std::filesystem::remove(file_path);
                    }
                    std::rethrow_exception(p);
                } catch (const std::exception& e) {
                    NcmImageProviderInner::handle_res(
                        rsp_guard,
                        nstd::unexpected(To<QString>::from(
                            fmt::format("NcmImageProvider, id: {}, error: {}", id, e.what()))));
                }
            }
        });
    return rsp;
}
