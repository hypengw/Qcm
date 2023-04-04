#include "Qcm/ncm_image.h"

#include <filesystem>
#include <cstdio>
#include <fstream>

#include <ctre.hpp>
#include <QPointer>

#include "Qcm/app.h"
#include "Qcm/type.h"
#include "Qcm/path.h"

#include "core/expected_helper.h"
#include "request/response.h"
#include "asio_helper/sync_file.h"
#include "crypto/crypto.h"

using namespace qcm;

namespace
{
constexpr int DEF_SIZE = 400;

inline QSize get_down_size(const QSize& req) {
    if (req.width() <= DEF_SIZE) {
        double rate = req.height() / (double)req.width();
        if (rate < 1.0) {
            return { DEF_SIZE, (int)(DEF_SIZE * rate) };
        } else {
            return { (int)(DEF_SIZE / rate), DEF_SIZE };
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

asio::awaitable<request::Header> dl_image(ncm::Client cli, const request::Request& req,
                                          std::filesystem::path p) {
    helper::SyncFile file { std::fstream(p, std::ios::out | std::ios::binary) };
    file.handle().exceptions(std::ios_base::failbit | std::ios_base::badbit);

    auto rsp_http = co_await cli.rsp(req);
    co_await rsp_http->read_to_stream(file);

    file.handle().close();
    co_return rsp_http->header();
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

NcmImageProvider::NcmImageProvider(rc<CacheSql> cache)
    : QQuickAsyncImageProvider(),
      m_ex(App::instance()->get_pool_executor()),
      m_cli(App::instance()->ncm_client()),
      m_cache_sql(cache) {}

QQuickImageResponse* NcmImageProvider::requestImageResponse(const QString& id,
                                                            const QSize&   requestedSize) {
    NcmAsyncImageResponse* rsp = new NcmAsyncImageResponse();

    if (id.isEmpty()) {
        rsp->finished();
        return rsp;
    }

    auto ex = asio::make_strand(m_ex);

    ncm::Client cli       = m_cli;
    auto        rsp_guard = QPointer(rsp);
    auto        handle    = [rsp_guard](nstd::expected<QImage, QString> res) {
        if (res.has_value()) {
            QMetaObject::invokeMethod(
                rsp_guard, "handle", Qt::QueuedConnection, Q_ARG(QImage, res.value()));
        } else {
            QMetaObject::invokeMethod(
                rsp_guard, "handle_error", Qt::QueuedConnection, Q_ARG(QString, res.error()));
        }
    };
    request::Request      req       = NcmImageProvider::makeReq(id, requestedSize, cli);
    std::filesystem::path file_path = NcmImageProvider::genImageCachePath(req);
    auto                  cache     = m_cache_sql;

    asio::co_spawn(
        ex,
        rsp->wdog().watch(
            ex,
            [handle, cli, requestedSize, req, file_path, cache]() -> asio::awaitable<void> {
                auto key = file_path.filename().native();
                asio::co_spawn(cache->get_executor(), cache->get(key), asio::detached);
                if (! std::filesystem::exists(file_path)) {
                    CacheSql::Item db_it;
                    db_it.key = key;

                    auto file_path_dl = file_path;
                    file_path_dl.replace_extension(
                        fmt::format("dl{}x{}", requestedSize.width(), requestedSize.height()));

                    auto header = co_await dl_image(cli, req, file_path_dl);
                    std::filesystem::rename(file_path_dl, file_path);
                    header_record_db(header, db_it);
                    co_await cache->insert(db_it);
                }
                auto img = QImage(file_path.c_str());
                if (requestedSize.isValid()) {
                    img = img.scaled(requestedSize);
                }
                handle(img);
                co_return;
            }),
        [handle, file_path](std::exception_ptr p) {
            if (p) {
                try {
                    if (std::filesystem::exists(file_path)) {
                        std::filesystem::remove(file_path);
                    }
                    std::rethrow_exception(p);
                } catch (const std::exception& e) {
                    handle(nstd::unexpected(To<QString>::from(e.what())));
                }
            }
        });
    return rsp;
}
