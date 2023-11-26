#include "Qcm/app.h"

#include <cmath>
#include <array>

#include <QQuickWindow>
#include <QQuickStyle>
#include <QGlobalStatic>
#include <qapplicationstatic.h>
#include <QSettings>
#include <QJSValueIterator>
#include <QQuickItem>

#include <asio/deferred.hpp>

#include "Qcm/path.h"
#include "Qcm/type.h"
#include "Qcm/ncm_image.h"
#include "Qcm/qr_image.h"
#include "ncm/api/user_account.h"
#include "crypto/crypto.h"
#include "Qcm/info.h"
#include "Qcm/cache_sql.h"

#include "request/response.h"
#include "asio_helper/sync_file.h"
#include "platform/platform.h"

#include "meta_model/qgadget_helper.h"

using namespace qcm;

namespace
{

asio::awaitable<void> scan_media_cache(rc<CacheSql> cache_sql, std::filesystem::path cache_dir) {
    auto                  cache_entries = co_await cache_sql->get_all();
    std::set<std::string> keys, files;
    for (auto& el : cache_entries) keys.insert(el.key);
    for (auto& el : std::filesystem::directory_iterator(cache_dir))
        files.insert(el.path().filename());

    for (auto& k : keys) {
        if (! files.contains(k)) {
            co_await cache_sql->remove(k);
        }
    }

    for (auto& f : files) {
        if (! keys.contains(f)) {
            std::filesystem::remove(cache_dir / f);
        }
    }

    co_await cache_sql->try_clean();
    co_return;
}

} // namespace

Q_APPLICATION_STATIC(App, app);

App* App::instance() { return app; }

App::App()
    : QObject(nullptr),
      m_qt_ex(std::make_shared<QtExecutionContext>(this)),
      m_pool(6),
      m_session(std::make_shared<request::Session>(m_pool.get_executor())),
      m_client(m_session, m_pool.get_executor()),
      m_mpris(std::make_unique<mpris::Mpris>()),
      m_media_cache(std::make_shared<media_cache::MediaCache>(m_pool.get_executor(), m_session)) {
    QGuiApplication::setApplicationName(AppName.data());
    QGuiApplication::setOrganizationName(AppName.data());
    QGuiApplication::setDesktopFileName(APP_ID);
    // QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

    m_media_cache_sql = std::make_shared<CacheSql>("media_cache", 0);
    m_cache_sql       = std::make_shared<CacheSql>("cache", 0);
}
App::~App() {
    save_session();
    m_media_cache->stop();
    m_session->about_to_stop();
    m_pool.join();
}

ncm::Client App::ncm_client() const { return m_client; }

void App::init(QQmlApplicationEngine* engine) {
    qmlRegisterSingletonInstance("Qcm.App", 1, 0, "App", this);
    qcm::init_path(std::array { config_path() / "session", data_path() });

    {
        m_mpris->registerService("Qcm");
        auto m = mpris();
        m->setIdentity("Qcm");
        m->setDesktopEntry(APP_ID); // no ".desktop"
        m->setCanQuit(true);
        qmlRegisterUncreatableType<mpris::MediaPlayer2>(
            "Qcm.App", 1, 0, "MprisMediaPlayer", "uncreatable");
    }

    {
        auto cache_dir = cache_path() / "cache";
        m_cache_sql->set_clean_cb([cache_dir](std::string_view key) {
            auto file = cache_dir / key;
            std::filesystem::remove(file);
            DEBUG_LOG("cache remove {}", file.native());
        });
        asio::co_spawn(
            m_cache_sql->get_executor(), scan_media_cache(m_cache_sql, cache_dir), asio::detached);
    }

    {
        auto media_cache_dir = cache_path() / "media";
        m_media_cache_sql->set_clean_cb([media_cache_dir](std::string_view key) {
            auto file = media_cache_dir / key;
            std::filesystem::remove(file);
            DEBUG_LOG("cache remove {}", file.native());
        });

        m_media_cache->start(media_cache_dir, m_media_cache_sql);
        asio::co_spawn(m_media_cache_sql->get_executor(),
                       scan_media_cache(m_media_cache_sql, media_cache_dir),
                       asio::detached);
    }
    triggerCacheLimit();

    load_session();

    QQuickStyle::setStyle("Material");

    auto gui_app = QGuiApplication::instance();

    connect(engine, &QQmlApplicationEngine::quit, gui_app, &QGuiApplication::quit);

    engine->addImageProvider(u"ncm"_qs, new NcmImageProvider {});
    engine->addImageProvider(u"qr"_qs, new QrImageProvider {});

    engine->load(u"qrc:/main/main.qml"_qs);
}

model::ArtistId App::artistId(QString id) const { return { id }; }
model::AlbumId  App::albumId(QString id) const { return { id }; }

QUrl App::getImageCache(QString url, QSize reqSize) const {
    auto path =
        NcmImageProvider::genImageCachePath(NcmImageProvider::makeReq(url, reqSize, m_client));
    return QUrl::fromLocalFile(path.native().c_str());
}

QUrl App::media_file(const QString& id_) const {
    auto id              = convert_from<std::string>(id_);
    auto media_cache_dir = cache_path() / "media";
    asio::co_spawn(m_media_cache_sql->get_executor(), m_media_cache_sql->get(id), asio::detached);
    auto file = media_cache_dir / id;
    if (std::filesystem::exists(file))
        return QUrl::fromLocalFile(convert_from<QString>(file.native()));
    return {};
}

QString App::media_url(const QString& ori, const QString& id) const {
    return convert_from<QString>(
        m_media_cache->get_url(convert_from<std::string>(ori), convert_from<std::string>(id)));
}

QString App::md5(QString txt) const {
    auto opt = crypto::digest(crypto::md5(), convert_from<std::vector<byte>>(txt.toStdString()))
                   .map([](auto in) {
                       return convert_from<QString>(crypto::hex::encode_low(in));
                   });
    _assert_(opt);
    return std::move(opt).value();
}

void App::loginPost(model::UserAccount* user) {
    auto& id = user->m_userId;
    if (id.valid()) {
        QSettings s;
        s.setValue("session/user_id", convert_from<QString>((fmt::format("ncm-{}", id.id))));

        save_session();
    }
}

void App::triggerCacheLimit() {
    QSettings                  s;
    constexpr double           def_total_cache_limit { 3.0 * 1024 * 1024 * 1024 };
    constexpr double           def_medie_cache_limit { 2.0 * 1024 * 1024 * 1024 };
    constexpr std::string_view total_cache_key { "cache/total_cache_limit" };
    constexpr std::string_view media_cache_key { "cache/media_cache_limit" };
    if (! s.contains(total_cache_key)) {
        s.setValue(total_cache_key, def_total_cache_limit);
    }
    if (! s.contains(media_cache_key)) {
        s.setValue(media_cache_key, def_medie_cache_limit);
    }

    auto media_cache_limit = s.value(media_cache_key).toDouble() / 1024;
    m_media_cache_sql->set_limit(media_cache_limit);

    auto limit = s.value(total_cache_key).toDouble() / 1024 - media_cache_limit;
    m_cache_sql->set_limit(limit);
}

void App::load_session() {
    QSettings s;
    auto      user_id = convert_from<std::string>(s.value("session/user_id").toString());
    if (! user_id.empty()) {
        m_session->load_cookie(config_path() / "session" / user_id);
    }
}

void App::save_session() {
    QSettings s;
    auto      user_id = convert_from<std::string>(s.value("session/user_id").toString());
    if (! user_id.empty()) {
        m_session->save_cookie(config_path() / "session" / user_id);
    }
}

bool App::isItemId(const QJSValue& v) const {
    auto var = v.toVariant();
    if (var.isValid()) {
        auto meta = var.metaType().metaObject();
        return meta != nullptr && meta->inherits(&model::ItemId::staticMetaObject);
    }
    return false;
}

void App::test() {
    /*
    asio::co_spawn(
        m_session->get_strand(),
        [this]() -> asio::awaitable<void> {
            helper::SyncFile file { std::fstream("/var/tmp/test.iso",
                                                 std::ios::out | std::ios::binary) };
            file.handle().exceptions(std::ios_base::failbit | std::ios_base::badbit);

            request::Request req;
            req.set_url("https://mirrors.aliyun.com/ubuntu-releases/jammy/"
                        "ubuntu-22.04.2-desktop-amd64.iso")
                .set_header("user-agent", "curl/7.87.0");
            auto rsp = co_await m_session->get(req);
            co_await rsp.value()->read_to_stream(file);
            co_return;
        },
        asio::detached);
    */
}

bool App::debug() const {
#ifndef NDEBUG
    return 1;
#else
    return 0;
#endif
}

model::Song  App::song(const QJSValue& js) const { return meta_model::toGadget<model::Song>(js); }
model::Album App::album(const QJSValue& js) const { return meta_model::toGadget<model::Album>(js); }
model::Artist App::artist(const QJSValue& js) const {
    return meta_model::toGadget<model::Artist>(js);
}
model::Djradio App::djradio(const QJSValue& js) const {
    return meta_model::toGadget<model::Djradio>(js);
}
model::Playlist App::playlist(const QJSValue& js) const {
    return meta_model::toGadget<model::Playlist>(js);
}
model::Program App::program(const QJSValue& js) const {
    return meta_model::toGadget<model::Program>(js);
}

QString App::itemIdPageUrl(const QJSValue& js) const {
    auto variant = js.toVariant();
    if (variant.canConvert<model::AlbumId>()) {
        return "qrc:/Qcm/App/qml/page/AlbumDetailPage.qml";
    } else if (variant.canConvert<model::ArtistId>()) {
        return "qrc:/Qcm/App/qml/page/ArtistDetailPage.qml";
    } else if (variant.canConvert<model::PlaylistId>()) {
        return "qrc:/Qcm/App/qml/page/PlaylistDetailPage.qml";
    } else if (variant.canConvert<model::DjradioId>()) {
        return "qrc:/Qcm/App/qml/page/DjradioDetailPage.qml";
    }
    return {};
}

// #include <private/qquickpixmapcache_p.h>
void App::releaseResources(QQuickWindow* win) {
    win->releaseResources();
    // QQuickPixmap::purgeCache();
    plt::malloc_trim(0);
}

QSize App::image_size(QSize display, int quality, QQuickItem* item) const {
    QSize out;
    if (quality == ImgOrigin) {
        out = { -1, -1 };
    } else if (quality == ImgAuto) {
        constexpr std::array sizes { Img400px, Img800px, Img1200px };
        auto                 size = std::max(display.width(), display.height());
        auto dpr = (item && item->window() ? item->window()->effectiveDevicePixelRatio() : 1.0);
        size = size * (item && item->window() ? item->window()->effectiveDevicePixelRatio() : 1.0);
        auto it = std::upper_bound(sizes.begin(), sizes.end(), size);
        size    = it == sizes.end() ? -1 : (int)((int)*it / dpr);
        out     = display.scaled(size, size, Qt::AspectRatioMode::KeepAspectRatio);
    } else {
        out = display.scaled(quality, quality, Qt::AspectRatioMode::KeepAspectRatio);
    }
    return out;
}