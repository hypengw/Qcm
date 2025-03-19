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

#include "asio_helper/basic.h"
#include "core/helper.h"
#include "core/log.h"
#include "core/qvariant_helper.h"

#include "crypto/crypto.h"
#include "asio_helper/sync_file.h"
#include "meta_model/qgadget_helper.h"
import platform;
#include "asio_qt/qt_sql.h"

#include "qcm_interface/plugin.h"
#include "qcm_interface/type.h"
#include "qcm_interface/path.h"

#include "Qcm/qr_image.h"
#include "Qcm/image_provider.h"
#include "Qcm/qml_util.h"
#include "Qcm/sql/collection_sql.h"
#include "Qcm/sql/cache_sql.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/info.h"

#include "media_cache/media_cache.h"
#include "qcm_interface/model/user_account.h"
#include "Qcm/play_queue.h"
#include "Qcm/backend.h"
#include "Qcm/player.h"
#include "Qcm/status/provider_status.hpp"

#ifndef NODEBUS
#    include "mpris/mpris.h"
#    include "mpris/mediaplayer2.h"
#endif

using namespace qcm;

DEFINE_CONVERT(ncrequest::req_opt::Proxy::Type, App::ProxyType) {
    out = static_cast<std::decay_t<decltype(out)>>(in);
}

namespace
{

void cache_clean_cb(const std::filesystem::path& cache_dir, std::string_view key) {
    std::error_code ec;
    auto            file = cache_dir / (key.size() >= 2 ? key.substr(0, 2) : "no"sv) / key;
    std::filesystem::remove(file, ec);
    log::debug("cache remove {}", file.native());
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

    log::debug(R"(
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

namespace qcm
{

class App::Private {
public:
    Private(App* self)
        : m_p(self),
          m_global(make_rc<Global>()),
          m_util(make_rc<qml::Util>(std::monostate {})),
          m_play_id_queue(new PlayIdQueue(self)),
          m_playqueu(new qcm::PlayQueue(self)),
          m_empty(new qcm::model::EmptyModel(self)),
          provider_status(new ProviderStatusModel(self)),
#ifndef NODEBUS
          m_mpris(make_up<mpris::Mpris>()),
#endif
          m_media_cache(),
          m_main_win(nullptr),
          m_qml_engine(make_up<QQmlApplicationEngine>()) {
    }
    ~Private() {
        m_qml_engine = nullptr;

        m_p->save_settings();
        m_media_cache->stop();
        m_global->session()->about_to_stop();
        m_global->join();
    }

    App* m_p;

    Arc<Global>    m_global;
    Arc<qml::Util> m_util;

    PlayIdQueue*         m_play_id_queue;
    PlayQueue*           m_playqueu;
    model::EmptyModel*   m_empty;
    ProviderStatusModel* provider_status;
#ifndef NODEBUS
    Box<mpris::Mpris> m_mpris;
#endif

    Arc<media_cache::MediaCache> m_media_cache;

    Arc<CacheSql>      m_media_cache_sql;
    Arc<CacheSql>      m_cache_sql;
    Arc<CollectionSql> m_collect_sql;
    Arc<ItemSql>       m_item_sql;

    Box<Backend> m_backend;

    std::optional<Sender<Player::NotifyInfo>> m_player_sender;
    QPointer<QQuickWindow>                    m_main_win;
    Box<QQmlApplicationEngine>                m_qml_engine;
};

App* App::create(QQmlEngine*, QJSEngine*) {
    auto app = app_instance();
    // not delete on qml
    QJSEngine::setObjectOwnership(app, QJSEngine::CppOwnership);
    return app;
}

App* App::instance() { return app_instance(); }

App::App(QStringView backend_exe, std::monostate)
    : QObject(nullptr), d_ptr(make_box<Private>(this)) {
    C_D(App);
    app_instance(this);
    register_meta_type();
    connect_actions();
    {
        QGuiApplication::setDesktopFileName(APP_ID);
    }
    d->m_playqueu->setSourceModel(d->m_play_id_queue);
    {
        auto fbs         = make_rc<media_cache::Fallbacks>();
        d->m_media_cache = make_rc<media_cache::MediaCache>(
            d->m_global->pool_executor(), d->m_global->session(), fbs);
    }

    log::debug("thread pool size: {}", get_pool_size());
    {
        d->m_backend  = make_box<Backend>();
        auto data_dir = convert_from<QString>(data_path().string());
        d->m_backend->start(backend_exe, data_dir);
    }

    d->m_qml_engine->addImportPath(u"qrc:/"_s);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    // try curve for better good visual results or where reducing graphics memory consumption
    QQuickWindow::setTextRenderType(QQuickWindow::CurveTextRendering);
#endif

    // sql init
    {
        auto cache_db        = make_rc<helper::SqlConnect>(data_path() / "cache.db", u"cache");
        auto data_db         = make_rc<helper::SqlConnect>(data_path() / "data.db", u"data");
        d->m_media_cache_sql = make_rc<CacheSql>("media_cache", 0, cache_db);
        d->m_cache_sql       = make_rc<CacheSql>("cache", 0, cache_db);
        d->m_item_sql        = make_rc<ItemSql>(data_db);
        d->m_collect_sql     = make_rc<CollectionSql>("collection", data_db);
        d->m_global->set_cache_sql(d->m_cache_sql);
        d->m_global->set_metadata_impl(player::get_metadata);
        d->m_global->set_collection_sql(d->m_collect_sql);
        d->m_global->set_item_sql(d->m_item_sql);
    }
}
App::~App() {}

void App::init() {
    C_D(App);
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
#ifndef NODEBUS
    {
        d->m_mpris->registerService("Qcm");
        auto m = d->m_mpris->mediaplayer2();
        m->setIdentity("Qcm");
        m->setDesktopEntry(APP_ID); // no ".desktop"
        m->setCanQuit(true);
        qmlRegisterUncreatableType<mpris::MediaPlayer2>(
            "Qcm.App", 1, 0, "MprisMediaPlayer", "uncreatable");
    }
#endif

    // cache
    {
        auto cache_dir = cache_path() / "cache";
        d->m_cache_sql->set_clean_cb([cache_dir](std::string_view key) {
            cache_clean_cb(cache_dir, key);
        });
        asio::co_spawn(d->m_cache_sql->get_executor(),
                       scan_media_cache(d->m_cache_sql, cache_dir),
                       asio::detached);
    }

    // media cache
    {
        auto media_cache_dir = cache_path() / "media";
        d->m_media_cache_sql->set_clean_cb([media_cache_dir](std::string_view key) {
            cache_clean_cb(media_cache_dir, key);
        });

        d->m_media_cache->start(media_cache_dir, d->m_media_cache_sql);
        asio::co_spawn(d->m_media_cache_sql->get_executor(),
                       scan_media_cache(d->m_media_cache_sql, media_cache_dir),
                       asio::detached);
    }
    triggerCacheLimit();

    load_settings();

    auto gui_app = QGuiApplication::instance();

    connect(engine, &QQmlApplicationEngine::quit, gui_app, &QGuiApplication::quit);

    engine->addImageProvider(u"qr"_s, new QrImageProvider {});

    load_plugins();

    // default delegate var
    {
        engine->rootContext()->setContextProperty("index", 0);
        engine->rootContext()->setContextProperty("model", QVariant::fromValue(nullptr));
        engine->rootContext()->setContextProperty("modelData", QVariant::fromValue(nullptr));
    }

    engine->addImageProvider("qcm", new QcmImageProvider);

    engine->load(u"qrc:/main/main.qml"_s);

    for (auto el : engine->rootObjects()) {
        if (auto win = qobject_cast<QQuickWindow*>(el)) {
            d->m_main_win = win;
        }
    }

    _assert_msg_rel_(d->m_main_win, "main window must exist");
    _assert_msg_rel_(d->m_player_sender, "player must init");

    global()->load_user();

    if (false && global()->user_model()->active_user() != nullptr) {
        auto user = global()->user_model()->active_user();
        global()->switch_user(user);
    } else {
        global()->app_state()->set_state(state::AppState::Start {});
    }
}

QString App::media_url(const QUrl& ori, const QString& id) const {
    C_D(const App);
    return convert_from<QString>(d->m_media_cache->get_url(
        convert_from<std::string>(ori.toString()), convert_from<std::string>(id)));
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
    C_D(App);
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
    d->m_media_cache_sql->set_limit(media_cache_limit);

    auto limit = s.value(total_cache_key).toDouble() / 1024 - media_cache_limit;
    d->m_cache_sql->set_limit(limit);
}

void App::load_plugins() {
    C_D(App);
    // init all static plugins
    QPluginLoader::staticInstances();
}

void App::setProxy(ProxyType t, QString content) {
    C_D(App);
    d->m_global->session()->set_proxy(
        ncrequest::req_opt::Proxy { .type    = convert_from<ncrequest::req_opt::Proxy::Type>(t),
                                    .content = content.toStdString() });
}

void App::setVerifyCertificate(bool v) {
    C_D(App);
    d->m_global->session()->set_verify_certificate(v);
}

bool App::isItemId(const QJSValue& v) const {
    auto var = v.toVariant();
    if (var.isValid()) {
        auto meta = var.metaType().metaObject();
        return meta != nullptr && meta->inherits(&model::ItemId::staticMetaObject);
    }
    return false;
}

QVariant App::import_path_list() {
    C_D(App);
    return d->m_qml_engine->importPathList();
}

void App::test() {
    /*
    asio::co_spawn(
        d->m_session->get_strand(),
        [this]() -> asio::awaitable<void> {
            helper::SyncFile file { std::fstream("/var/tmp/test.iso",
                                                 std::ios::out | std::ios::binary) };
            file.handle().exceptions(std::ios_base::failbit | std::ios_base::badbit);

            ncrequest::Request req;
            req.set_url("https://mirrors.aliyun.com/ubuntu-releases/jammy/"
                        "ubuntu-22.04.2-desktop-amd64.iso")
                .set_header("user-agent", "curl/7.87.0");
            auto rsp = co_await d->m_session->get(req);
            co_await rsp.value()->read_to_stream(file);
            co_return;
        },
        asio::detached);
    */
}

auto App::mpris() const -> QObject* {
    C_D(const App);
#ifndef NODEBUS
    return d->m_mpris->mediaplayer2();
#else
    return nullptr;
#endif
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
        return "qrc:/Qcm/App/qml/page/MixDetailPage.qml";
    } else if (type == "radio") {
        return "qrc:/Qcm/App/qml/page/RadioDetailPage.qml";
    }
    return {};
}

auto App::engine() const -> QQmlApplicationEngine* {
    C_D(const App);
    return d->m_qml_engine.get();
}
auto App::global() const -> Global* {
    C_D(const App);
    return d->m_global.get();
}
auto App::backend() const -> Backend* {
    C_D(const App);
    return d->m_backend.get();
}
auto App::util() const -> qml::Util* {
    C_D(const App);
    return d->m_util.get();
}
auto App::playqueue() const -> PlayQueue* {
    C_D(const App);
    return d->m_playqueu;
}
auto App::play_id_queue() const -> PlayIdQueue* {
    C_D(const App);
    return d->m_play_id_queue;
}

// #include <private/qquickpixmapcache_p.h>
void App::releaseResources(QQuickWindow*) {
    C_D(const App);
    log::info("gc");
    // win->releaseResources();
    d->m_qml_engine->trimComponentCache();
    d->m_qml_engine->collectGarbage();
    // QQuickPixmap::purgeCache();
    plt::malloc_trim(0);
    auto as_mb = [](usize n) {
        return fmt::format("{:.2f} MB", n / (1024.0 * 1024.0));
    };
    auto info = plt::mem_info();
    log::debug(R"(
heap: {}
mmap({}): {}
in use: {}
dyn create: {}
img rsp: {}
)",
              as_mb(info.heap),
              info.mmap_num,
              as_mb(info.mmap),
              as_mb(info.totle_in_use),
              qml_dyn_count().load(),
              image_response_count().load());
}

qreal App::devicePixelRatio() const {
    C_D(const App);
    return d->m_main_win ? d->m_main_win->effectiveDevicePixelRatio() : qApp->devicePixelRatio();
}

QSizeF App::image_size(QSizeF display, int quality, QQuickItem* item) const {
    C_D(const App);
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
    C_D(App);
    d->m_player_sender = sender;
    d->m_media_cache->fallbacks()->fragment =
        [sender](usize begin, usize end, usize totle) mutable {
            if (totle >= end && totle >= begin) {
                float db = begin / (double)totle;
                float de = end / (double)totle;
                sender.try_send(player::notify::cache { db, de });
            }
        };
}

auto App::media_cache_sql() const -> rc<CacheSql> {
    C_D(const App);
    return d->m_media_cache_sql;
}
auto App::cache_sql() const -> rc<CacheSql> {
    C_D(const App);
    return d->m_cache_sql;
}
auto App::item_sql() const -> rc<ItemSql> {
    C_D(const App);
    return d->m_item_sql;
}
auto App::collect_sql() const -> rc<CollectionSql> {
    C_D(const App);
    return d->m_collect_sql;
}
auto App::empty() const -> model::EmptyModel* {
    C_D(const App);
    return d->m_empty;
}
auto App::provider_status() const -> ProviderStatusModel* {
    C_D(const App);
    return d->provider_status;
}
void App::switchPlayIdQueue() {
    C_D(App);
    d->m_playqueu->setSourceModel(d->m_play_id_queue);
}

void App::load_settings() {
    C_D(App);
    QSettings s;
    playqueue()->setLoopMode(s.value("play/loop").value<enums::LoopMode>());
    connect(playqueue(), &PlayQueue::loopModeChanged, this, [](enums::LoopMode v) {
        QSettings s;
        s.setValue("play/loop", (int)v);
    });
    playqueue()->setRandomMode(s.value("play/random").value<bool>());
    connect(playqueue(), &PlayQueue::randomModeChanged, this, [](bool v) {
        QSettings s;
        s.setValue("play/random", v);
    });
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
    s.setValue("play/loop", (int)playqueue()->loopMode());
}
} // namespace qcm

#include "Qcm/query/mix_detail.h"

void qcm::register_meta_type() { qRegisterMetaType<query::MixDetailQuery>(); }

#include <Qcm/moc_app.cpp>