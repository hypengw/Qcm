#include "Qcm/app.hpp"

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
#include <QQmlContext>

#include "core/asio/basic.h"
#include "core/helper.h"
#include "core/log.h"
#include "core/qvariant_helper.h"

#include "crypto/crypto.h"
#include "core/asio/sync_file.h"
#include "meta_model/qgadget_helper.hpp"
import platform;

#include "Qcm/util/path.hpp"

#include "Qcm/image_provider/http.hpp"
#include "Qcm/image_provider/qr.hpp"
#include "Qcm/qml/qml_util.hpp"

#include "Qcm/model/play_queue.hpp"
#include "Qcm/model/page_model.hpp"
#include "Qcm/store.hpp"
#include "Qcm/backend.hpp"
#include "Qcm/player.hpp"
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
          provider_meta_status(new ProviderMetaStatusModel(self)),
          provider_status(new ProviderStatusModel(self)),
          page_model(new PageModel(self)),
          app_state(new AppState(self)),
#ifndef NODEBUS
          m_mpris(make_up<mpris::Mpris>()),
#endif
          m_main_win(nullptr),
          m_qml_engine(make_up<QQmlApplicationEngine>()) {
    }
    ~Private() {
        m_qml_engine = nullptr;

        m_p->save_settings();
        m_global->session()->about_to_stop();
        m_global->join();
    }

    App* m_p;

    Arc<Global>    m_global;
    Arc<qml::Util> m_util;

    PlayIdQueue*             m_play_id_queue;
    PlayQueue*               m_playqueu;
    model::EmptyModel*       m_empty;
    ProviderMetaStatusModel* provider_meta_status;
    ProviderStatusModel*     provider_status;
    PageModel*               page_model;
    model::AppInfo           info;
    AppState*                app_state;

#ifndef NODEBUS
    Box<mpris::Mpris> m_mpris;
#endif

    Box<Backend> m_backend;

    std::optional<Sender<Player::NotifyInfo>> m_player_sender;
    QPointer<QQuickWindow>                    m_main_win;
    Box<QQmlApplicationEngine>                m_qml_engine;
};

App* App::create(QQmlEngine*, QJSEngine*) {
    auto app = app_instance();
    // not delete by qml
    QJSEngine::setObjectOwnership(app, QJSEngine::CppOwnership);
    return app;
}

App* App::instance() { return app_instance(); }

App::App(QStringView backend_exe, std::monostate)
    : QObject(nullptr), d_ptr(make_box<Private>(this)) {
    C_D(App);
    app_instance(this);
    // register_meta_type();
    connect_actions();
    {
        QGuiApplication::setDesktopFileName(APP_ID);
        PageModel::init_main_pages(d->page_model);
    }
    d->m_playqueu->setSourceModel(d->m_play_id_queue);
    {
    }

    log::debug("thread pool size: {}", get_pool_size());
    {
        d->m_backend = ::make_box<Backend>(d->m_global->session());
        {
            connect(
                d->m_backend.get(),
                &Backend::error,
                d->app_state,
                [d](QString err) {
                    QObject::connect(
                        d->app_state, &AppState::retry, d->m_backend.get(), &Backend::on_retry);
                    d->app_state->set_state(AppState::Error { err });
                },
                Qt::QueuedConnection);
        }

        auto data_dir = convert_from<QString>(data_path().string());
        d->m_backend->start(backend_exe, data_dir);
    }

    d->m_qml_engine->addImportPath(u"qrc:/"_s);
    // curve text only suitable for 4K
    // QQuickWindow::setTextRenderType(QQuickWindow::CurveTextRendering);
    if (0) {
        // MSAA
        auto format = QSurfaceFormat {};
        format.setSamples(4);
        QSurfaceFormat::setDefaultFormat(format);
    }

    // sql init
    {
        d->m_global->set_metadata_impl(player::get_metadata);
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
    engine->loadFromModule("Qcm.App", "Window");

    for (auto el : engine->rootObjects()) {
        if (auto win = qobject_cast<QQuickWindow*>(el)) {
            d->m_main_win = win;
        }
    }

    _assert_msg_rel_(d->m_main_win, "main window must exist");
    _assert_msg_rel_(d->m_player_sender, "player must init");
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
    // auto  itemId = js.toVariant().value<model::ItemId>();
    // auto& type   = itemId.type();
    // if (type == "album") {
    //     return "qrc:/Qcm/App/qml/page/AlbumDetailPage.qml";
    // } else if (type == "artist") {
    //     return "qrc:/Qcm/App/qml/page/ArtistDetailPage.qml";
    // } else if (type == "playlist") {
    //     return "qrc:/Qcm/App/qml/page/MixDetailPage.qml";
    // } else if (type == "radio") {
    //     return "qrc:/Qcm/App/qml/page/RadioDetailPage.qml";
    // }
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
        return std::format("{:.2f} MB", n / (1024.0 * 1024.0));
    };

    auto store = AppStore::instance();

    log::debug(R"(
--- store ---
albums: {}
songs: {}
)",
               store->albums.size(),
               store->songs.size());

    auto info = plt::mem_info();
    log::debug(R"(
--- memory ---
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
               0,
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
    /*
    d->m_media_cache->fallbacks()->fragment =
        [sender](usize begin, usize end, usize totle) mutable {
            if (totle >= end && totle >= begin) {
                float db = begin / (double)totle;
                float de = end / (double)totle;
                sender.try_send(player::notify::cache { db, de });
            }
        };
        */
}

auto App::empty() const -> model::EmptyModel* {
    C_D(const App);
    return d->m_empty;
}
auto App::provider_meta_status() const -> ProviderMetaStatusModel* {
    C_D(const App);
    return d->provider_meta_status;
}
auto App::provider_status() const -> ProviderStatusModel* {
    C_D(const App);
    return d->provider_status;
}

auto App::pages() const -> PageModel* {
    C_D(const App);
    return d->page_model;
}
auto App::store() const -> AppStore* {
    C_D(const App);
    return AppStore::instance();
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

auto App::info() const -> const model::AppInfo& {
    C_D(const App);
    return d->info;
}

auto App::app_state() const -> AppState* {
    C_D(const App);
    return d->app_state;
}

} // namespace qcm

#include <Qcm/moc_app.cpp>