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
#include <QQmlContext>

#include <asio/deferred.hpp>

#include "core/qvariant_helper.h"
#include "crypto/crypto.h"
#include "request/response.h"
#include "asio_helper/sync_file.h"
#include "meta_model/qgadget_helper.h"
#include "platform/platform.h"

#include "qcm_interface/plugin.h"
#include "qcm_interface/type.h"
#include "qcm_interface/path.h"

#include "Qcm/qr_image.h"
#include "Qcm/image_provider.h"
#include "Qcm/qml_util.h"
#include "Qcm/collection_sql.h"
#include "Qcm/cache_sql.h"
#include "Qcm/info.h"

using namespace qcm;

DEFINE_CONVERT(request::req_opt::Proxy::Type, App::ProxyType) {
    out = static_cast<std::decay_t<decltype(out)>>(in);
}

namespace
{

void cache_clean_cb(const std::filesystem::path& cache_dir, std::string_view key) {
    std::error_code ec;
    auto            file = cache_dir / (key.size() >= 2 ? key.substr(0, 2) : "no"sv) / key;
    std::filesystem::remove(file, ec);
    DEBUG_LOG("cache remove {}", file.native());
}

asio::awaitable<void> scan_media_cache(rc<CacheSql> cache_sql, std::filesystem::path cache_dir) {
    auto                  cache_entries = co_await cache_sql->get_all();
    std::set<std::string> keys;
    // name,dirname
    std::map<std::string, std::string> files;
    std::vector<std::filesystem::path> useless_files;
    std::error_code                    ec;
    for (auto& el : cache_entries) keys.insert(el.key);
    for (auto& el : std::filesystem::directory_iterator(cache_dir)) {
        if (el.is_regular_file()) {
            useless_files.push_back(el);
        } else if (el.is_directory()) {
            auto dir = el.path();
            for (auto& el : std::filesystem::directory_iterator(el)) {
                if (el.is_regular_file()) {
                    files.insert({ el.path().filename(), dir.filename() });
                }
            }
        }
    }

    usize entry_removed { 0 }, file_removed { 0 };

    {
        std::vector<std::string> delete_keys;
        for (auto& k : keys) {
            if (! files.contains(k)) {
                delete_keys.push_back(k);
            }
        }
        co_await cache_sql->remove(delete_keys);
        entry_removed = delete_keys.size();
    }

    for (auto& f : files) {
        if (! keys.contains(f.first)) {
            std::filesystem::remove(cache_dir / f.second / f.first, ec);
            file_removed++;
        }
    }
    for (auto& f : useless_files) {
        std::filesystem::remove(f, ec);
    }
    file_removed += useless_files.size();

    co_await cache_sql->try_clean();

    DEBUG_LOG(R"(
cache cleaned:
    path: {}
    entry removed: {}
    file  removed: {}
    )",
              cache_dir.string(),
              entry_removed,
              file_removed);
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
      m_util(make_rc<qml::Util>(std::monostate {})),
      m_playlist(new qcm::Playlist(this)),
      m_mpris(make_up<mpris::Mpris>()),
      m_media_cache(),
      m_main_win(nullptr),
      m_qml_engine(make_up<QQmlApplicationEngine>()) {
    app_instance(this);
    register_meta_type();
    connect_actions();
    { QGuiApplication::setDesktopFileName(APP_ID); }
    {
        auto fbs = make_rc<media_cache::Fallbacks>();
        m_media_cache =
            make_rc<media_cache::MediaCache>(m_global->pool_executor(), m_global->session(), fbs);
    }

    DEBUG_LOG("thread pool size: {}", get_pool_size());

    m_qml_engine->addImportPath(u"qrc:/"_qs);
    // QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

    m_media_cache_sql = make_rc<CacheSql>("media_cache", 0);
    m_cache_sql       = make_rc<CacheSql>("cache", 0);
    m_collect_sql     = make_rc<CollectionSql>("collection");
    m_global->set_cache_sql(m_cache_sql);
    m_global->set_metadata_impl(player::get_metadata);
    m_global->set_collection_sql(m_collect_sql);
}
App::~App() {
    m_qml_engine = nullptr;

    save_settings();
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
            cache_clean_cb(cache_dir, key);
        });
        asio::co_spawn(
            m_cache_sql->get_executor(), scan_media_cache(m_cache_sql, cache_dir), asio::detached);
    }

    // media cache
    {
        auto media_cache_dir = cache_path() / "media";
        m_media_cache_sql->set_clean_cb([media_cache_dir](std::string_view key) {
            cache_clean_cb(media_cache_dir, key);
        });

        m_media_cache->start(media_cache_dir, m_media_cache_sql);
        asio::co_spawn(m_media_cache_sql->get_executor(),
                       scan_media_cache(m_media_cache_sql, media_cache_dir),
                       asio::detached);
    }
    triggerCacheLimit();

    load_settings();

    // qml engine
    QQuickStyle::setStyle("Material");

    auto gui_app = QGuiApplication::instance();

    connect(engine, &QQmlApplicationEngine::quit, gui_app, &QGuiApplication::quit);

    engine->addImageProvider(u"qr"_qs, new QrImageProvider {});

    load_plugins();

    // avoid listitem index reference error
    engine->rootContext()->setContextProperty("index", 0);
    engine->addImageProvider("qcm", new QcmImageProvider);

    engine->load(u"qrc:/main/main.qml"_qs);

    for (auto el : engine->rootObjects()) {
        if (auto win = qobject_cast<QQuickWindow*>(el)) {
            m_main_win = win;
        }
    }

    _assert_msg_rel_(m_main_win, "main window must exist");
    _assert_msg_rel_(m_player_sender, "player must init");

    global()->load_user();

    if (global()->user_model()->active_user() != nullptr) {
        auto user = global()->user_model()->active_user();
        global()->switch_user(user);
    } else {
        global()->app_state()->set_state(state::AppState::Start {});
    }
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
auto App::util() const -> qml::Util* { return m_util.get(); }
auto App::playlist() const -> Playlist* { return m_playlist; }

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

auto App::media_cache_sql() const -> rc<CacheSql> { return m_media_cache_sql; }
auto App::cache_sql() const -> rc<CacheSql> { return m_cache_sql; }

void App::load_settings() {
    QSettings s;
    playlist()->setLoopMode(s.value("play/loop").value<Playlist::LoopMode>());
    // session proxy
    {
        auto type        = s.value("network/proxy_type").value<ProxyType>();
        auto content     = s.value("network/proxy_content").toString();
        auto ignore_cert = convert_from<std::optional<bool>>(s.value("network/ignore_certificate"))
                               .value_or(false);

        setProxy(type, content);
        setVerifyCertificate(! ignore_cert);
    }
}
void App::save_settings() {
    QSettings s;
    s.setValue("play/loop", playlist()->loopMode());
}

void qcm::register_meta_type() {}
