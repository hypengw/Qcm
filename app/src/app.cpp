#include "Qcm/app.h"

#include <cmath>
#include <array>
#include <thread>

#include <QtCore/QGlobalStatic>
#include <qapplicationstatic.h>
#include <QtCore/QSettings>
#include <QtCore/QUuid>
#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtCore/QPluginLoader>

#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickItem>
#include <QtQml/QJSValueIterator>
#include <QtQml/QQmlEngineExtensionPlugin>
#include <QQuickStyle>

#include <asio/deferred.hpp>

#include "core/qvariant_helper.h"

#include "qcm_interface/type.h"

#include "qcm_interface/path.h"
#include "Qcm/qr_image.h"
#include "ncm/api/user_account.h"
#include "crypto/crypto.h"
#include "Qcm/info.h"
#include "Qcm/cache_sql.h"

#include "request/response.h"
#include "asio_helper/sync_file.h"
#include "platform/platform.h"

#include "meta_model/qgadget_helper.h"

#include "core/type.h"

using namespace qcm;

DEFINE_CONVERT(request::req_opt::Proxy::Type, App::ProxyType) {
    out = static_cast<std::decay_t<decltype(out)>>(in);
}

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

auto get_pool_size() -> std::size_t {
    return std::clamp<u32>(std::thread::hardware_concurrency(), 4, 12);
}

auto app_instance(App* in = nullptr) -> App* {
    static App* instance { in };
    _assert_msg_rel_(instance != nullptr, "app object not inited");
    _assert_msg_rel_(in == nullptr || instance == in, "there should be only one app object");
    return instance;
}

} // namespace

App* App::create(QQmlEngine*, QJSEngine*) {
    auto app = app_instance();
    // not delete on qml
    QJSEngine::setObjectOwnership(app, QJSEngine::CppOwnership);
    return app;
}

App* App::instance() { return app_instance(); }

App::App(std::monostate)
    : QObject(nullptr),
      m_global(make_rc<Global>()),
      m_mpris(make_up<mpris::Mpris>()),
      m_media_cache(),
      m_main_win(nullptr),
      m_qml_engine(make_up<QQmlApplicationEngine>()) {
    app_instance(this);
    { QGuiApplication::setDesktopFileName(APP_ID); }
    {
        auto fbs = make_rc<media_cache::Fallbacks>();
        m_media_cache =
            make_rc<media_cache::MediaCache>(m_global->pool_executor(), m_global->session(), fbs);
    }

    DEBUG_LOG("thread pool size: {}", get_pool_size());

    m_qml_engine->addImportPath(u"qrc:/"_qs);
    // QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

    m_media_cache_sql = std::make_shared<CacheSql>("media_cache", 0);
    m_cache_sql       = std::make_shared<CacheSql>("cache", 0);
    m_global->set_cache_sql(m_cache_sql);
    m_global->set_metadata_impl(player::get_metadata);
}
App::~App() {
    m_qml_engine = nullptr;

    save_session();
    m_media_cache->stop();
    m_global->session()->about_to_stop();
    m_global->join();
}

void App::init() {
    auto engine = this->engine();

    // qmlRegisterSingletonInstance("Qcm.App", 1, 0, "App", this);
    qcm::init_path(std::array { config_path() / "session", data_path() });

    // uuid
    {
        QSettings s;
        QUuid     uuid;
        auto      uuid_str = s.value("device/uuid").toString();
        if (uuid_str.isEmpty()) {
            uuid     = QUuid::createUuid();
            uuid_str = uuid.toString();
            s.setValue("device/uuid", uuid_str);
        } else {
            uuid = QUuid(uuid_str);
        }
        Global::instance()->set_uuid(uuid);
    }

    // mpris
    {
        m_mpris->registerService("Qcm");
        auto m = mpris();
        m->setIdentity("Qcm");
        m->setDesktopEntry(APP_ID); // no ".desktop"
        m->setCanQuit(true);
        qmlRegisterUncreatableType<mpris::MediaPlayer2>(
            "Qcm.App", 1, 0, "MprisMediaPlayer", "uncreatable");
    }

    // cache
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

    // media cache
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

    // session cookie
    load_session();

    // session proxy
    {
        QSettings s;
        auto      type    = s.value("network/proxy_type").value<ProxyType>();
        auto      content = s.value("network/proxy_content").toString();
        auto ignore_cert  = convert_from<std::optional<bool>>(s.value("network/ignore_certificate"))
                               .value_or(false);

        setProxy(type, content);
        setVerifyCertificate(! ignore_cert);
    }

    // qml engine
    QQuickStyle::setStyle("Material");

    auto gui_app = QGuiApplication::instance();

    connect(engine, &QQmlApplicationEngine::quit, gui_app, &QGuiApplication::quit);

    engine->addImageProvider(u"qr"_qs, new QrImageProvider {});

    load_plugins();

    global()->load_user();

    engine->load(u"qrc:/main/main.qml"_qs);

    for (auto el : engine->rootObjects()) {
        if (auto win = qobject_cast<QQuickWindow*>(el)) {
            m_main_win = win;
        }
    }

    _assert_msg_rel_(m_main_win, "main window must exist");
    _assert_msg_rel_(m_player_sender, "player must init");
}

QUrl App::getImageCache(QString provider, QUrl url, QSize reqSize) const {
    auto client = Global::instance()->client(provider.toStdString());
    if (client) {
        auto path = client.api->image_cache(client.instance, url, reqSize);
        return QUrl::fromLocalFile(path.c_str());
    }
    return {};
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
    auto& id = user->userId();
    if (id.valid()) {
        QSettings s;
        s.setValue("session/user_id",
                   convert_from<QString>(fmt::format("{}-{}", id.provider(), id.id())));

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
        m_global->session()->load_cookie(config_path() / "session" / user_id);
    }
}

void App::save_session() {
    QSettings s;
    auto      user_id = convert_from<std::string>(s.value("session/user_id").toString());
    if (! user_id.empty()) {
        m_global->session()->save_cookie(config_path() / "session" / user_id);
    }
}

void App::load_plugins() {
    std::optional<QDir> plugin_path;
    for (auto& el : this->engine()->importPathList()) {
        auto dir = QDir(el + QDir::separator() + "Qcm" + QDir::separator() + "Service");
        if (dir.exists()) {
            plugin_path = dir;
            break;
        }
    }

    if (plugin_path) {
        for (auto& dir_name : plugin_path->entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
            auto dir   = QDir(plugin_path->filePath(dir_name));
            auto files = dir.entryList(QDir::Filter::Files);
            if (auto it = std::find_if(files.begin(),
                                       files.end(),
                                       [](const QString& f) {
                                           return QLibrary::isLibrary(f);
                                       });
                it != files.end()) {
                auto plugin_so = dir.filePath(*it);
                if (this->global()->load_plugin(plugin_so.toStdString())) {
                    INFO_LOG("load plugin: {}", plugin_so);
                } else {
                    ERROR_LOG("load plugin failed: {}", plugin_so);
                }
            }
        }
    }
}

void App::setProxy(ProxyType t, QString content) {
    m_global->session()->set_proxy(request::req_opt::Proxy {
        .type = convert_from<request::req_opt::Proxy::Type>(t), .content = content.toStdString() });
}

void App::setVerifyCertificate(bool v) { m_global->session()->set_verify_certificate(v); }

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
    auto  itemId = js.toVariant().value<model::ItemId>();
    auto& type   = itemId.type();
    if (type == "album") {
        return "qrc:/Qcm/App/qml/page/AlbumDetailPage.qml";
    } else if (type == "artist") {
        return "qrc:/Qcm/App/qml/page/ArtistDetailPage.qml";
    } else if (type == "playlist") {
        return "qrc:/Qcm/App/qml/page/PlaylistDetailPage.qml";
    } else if (type == "djradio") {
        return "qrc:/Qcm/App/qml/page/DjradioDetailPage.qml";
    }
    return {};
}

auto App::engine() const -> QQmlApplicationEngine* { return m_qml_engine.get(); }
auto App::global() const -> Global* { return m_global.get(); }

// #include <private/qquickpixmapcache_p.h>
void App::releaseResources(QQuickWindow* win) {
    INFO_LOG("gc");
    win->releaseResources();
    m_qml_engine->trimComponentCache();
    m_qml_engine->collectGarbage();
    // QQuickPixmap::purgeCache();
    plt::malloc_trim(0);
    auto as_mb = [](usize n) {
        return fmt::format("{:.2f} MB", n / (1024.0 * 1024.0));
    };
    auto info = plt::mem_info();
    DEBUG_LOG(R"(
heap: {}
mmap({}): {}
in use: {}
dyn create: {}
)",
              as_mb(info.heap),
              info.mmap_num,
              as_mb(info.mmap),
              as_mb(info.totle_in_use),
              qml_dyn_count().load());
}

qreal App::devicePixelRatio() const {
    return m_main_win ? m_main_win->effectiveDevicePixelRatio() : qApp->devicePixelRatio();
}

QSizeF App::image_size(QSizeF display, int quality, QQuickItem* item) const {
    QSizeF out { -1, -1 };
    auto   dpr =
        (item && item->window() ? item->window()->effectiveDevicePixelRatio() : devicePixelRatio());

    if (quality == ImgOrigin) {
    } else if (quality == ImgAuto) {
        constexpr std::array sizes { Img400px, Img800px, Img1200px };
        auto                 size = std::max(display.width(), display.height()) * dpr;
        auto                 it   = std::upper_bound(sizes.begin(), sizes.end(), size);
        if (it != sizes.end()) {
            size = ((int)*it / dpr);
            out  = display.scaled(size, size, Qt::AspectRatioMode::KeepAspectRatio);
        }
    } else {
        qreal qualityF = quality / dpr;
        out            = display.scaled(qualityF, qualityF, Qt::AspectRatioMode::KeepAspectRatio);
    }
    return out;
}

auto App::bound_image_size(QSizeF displaySize) const -> QSizeF {
    auto size = std::max(displaySize.width(), displaySize.height());
    size      = 30 * (1 << (usize)std::ceil(std::log2(size / 30)));
    return displaySize.scaled(QSizeF(size, size), Qt::AspectRatioMode::KeepAspectRatioByExpanding);
}

void App::set_player_sender(Sender<Player::NotifyInfo> sender) {
    m_player_sender                      = sender;
    m_media_cache->fallbacks()->fragment = [sender](usize begin, usize end, usize totle) mutable {
        if (totle >= end && totle >= begin) {
            float db = begin / (double)totle;
            float de = end / (double)totle;
            sender.try_send(player::notify::cache { db, de });
        }
    };
}