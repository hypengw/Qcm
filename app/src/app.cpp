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
#include "kstore/qt/gadget_model.hpp"
import platform;

#include "Qcm/util/path.hpp"
#include "Qcm/util/mem.hpp"

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

DEFINE_CONVERT(ncrequest::req_opt::Proxy::Type, enums::ProxyType) {
    out = static_cast<std::decay_t<decltype(out)>>(in);
}

namespace
{

void cache_clean_cb(const std::filesystem::path& cache_dir, std::string_view key) {
    std::error_code ec;
    auto            file = cache_dir / (key.size() >= 2 ? key.substr(0, 2) : "no"sv) / key;
    std::filesystem::remove(file, ec);
    LOG_DEBUG("cache remove {}", file.string());
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
          m_util(make_rc<qml::Util>()),
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

        save_settings();
        m_playqueu->drop_global();
        m_global->join();
    }

    void save_settings() {
        QSettings s;
        s.setValue("play/loop", (int)m_playqueu->loopMode());
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
    register_meta_type();
    register_converters();
    connect_actions();
    connect_components();
    {
        QGuiApplication::setDesktopFileName(APP_ID);
        PageModel::init_main_pages(d->page_model);
        d->m_playqueu->setSourceModel(d->m_play_id_queue);
        d->m_global->set_metadata_impl(player::get_metadata);
        set_player_sender(d->m_global->player()->sender());
    }

    LOG_DEBUG("thread pool size: {}", get_pool_size());
    {
        d->m_backend = ::make_box<Backend>(d->m_global->session());
        {
            connect(
                d->m_backend.get(),
                &Backend::error,
                d->app_state,
                [d](QString err) {
                    QObject::disconnect(
                        d->app_state, &AppState::retry, d->m_backend.get(), &Backend::on_retry);
                    QObject::connect(
                        d->app_state, &AppState::retry, d->m_backend.get(), &Backend::on_retry);
                    d->app_state->set_state(AppState::Error { err });
                },
                Qt::QueuedConnection);
        }

        auto data_dir  = convert_from<QString>(data_path().string());
        auto cache_dir = convert_from<QString>(cache_path().string());
        d->m_backend->start(backend_exe, data_dir, cache_dir);
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
}
App::~App() {}

void App::init() {
    C_D(App);
    auto engine = this->engine();

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

void App::setProxy(enums::ProxyType t, QString content) {
    C_D(App);
    d->m_global->session()->set_proxy(
        ncrequest::req_opt::Proxy { .type    = convert_from<ncrequest::req_opt::Proxy::Type>(t),
                                    .content = content.toStdString() });
}

void App::setVerifyCertificate(bool v) {
    C_D(App);
    d->m_global->session()->set_verify_certificate(v);
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
void App::releaseResources(QQuickWindow*, const QJSValue& extra) {
    C_D(const App);
    LOG_INFO("gc");
    // win->releaseResources();
    d->m_qml_engine->trimComponentCache();
    d->m_qml_engine->collectGarbage();
    // QQuickPixmap::purgeCache();
    plt::malloc_trim(0);
    auto as_mb = [](usize n) {
        return std::format("{:.2f} MB", n / (1024.0 * 1024.0));
    };

    auto print_mem_stat = [as_mb](MemoryStatResource* s) {
        return std::format("used({}): {}, peak: {}",
                           s->current_block_count(),
                           as_mb(s->current_bytes()),
                           as_mb(s->peak_bytes()));
    };

    auto store = AppStore::instance();

    LOG_DEBUG(R"(
--- store ---
albums: {}
songs: {}
artists: {}
)",
               store->albums.size(),
               store->songs.size(),
               store->artists.size());

    auto info = plt::mem_info();
    LOG_DEBUG(R"(
--- memory ---
heap: {}
mmap({}): {}
in use: {}
pool obj: {}
img rsp: {}

pool:
  {}
session:
  {}
backend:
  {}
player:
  {}
store:
  {}
)",
               as_mb(info.heap),
               info.mmap_num,
               as_mb(info.mmap),
               as_mb(info.totle_in_use),
               extra.property("pool_obj").toInt(),
               image_response_count().load(),
               print_mem_stat(mem_mgr().pool_stat),
               print_mem_stat(mem_mgr().session_mem),
               print_mem_stat(mem_mgr().backend_mem),
               print_mem_stat(mem_mgr().player_mem),
               print_mem_stat(mem_mgr().store_mem));
}

qreal App::devicePixelRatio() const {
    C_D(const App);
    return d->m_main_win ? d->m_main_win->effectiveDevicePixelRatio() : qApp->devicePixelRatio();
}

auto App::bound_image_size(QSizeF displaySize) const -> QSizeF {
    auto size = std::max(displaySize.width(), displaySize.height());
    if (size == 0) return { 0.0f, 0.0f };
    size = 30 * (1 << (usize)std::ceil(std::log2(size / 30)));
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

auto App::libraryStatus() const -> LibraryStatus* {
    C_D(const App);
    return d->provider_status->libraryStatus();
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
    {
        const auto player = d->m_global->player();
        player->set_fadeTime(s.value("play/fade_time", player->fadeTime()).value<u32>());
        connect(player, &Player::fadeTimeChanged, this, [](u32 v) {
            QSettings s;
            s.setValue("play/fade_time", v);
        });

        player->set_volume(s.value("play/volume", player->volume()).value<float>());
        connect(player, &Player::volumeChanged, this, [](float v) {
            QSettings s;
            s.setValue("play/volume", v);
        });
    }

    // session proxy
    // {
    //     auto type        = s.value("network/proxy_type").value<enums::ProxyType>();
    //     auto content     = s.value("network/proxy_content").toString();
    //     auto ignore_cert =
    //     convert_from<std::optional<bool>>(s.value("network/ignore_certificate"))
    //                            .value_or(false);

    //     setProxy(type, content);
    //     setVerifyCertificate(! ignore_cert);
    // }
}
void App::save_settings() {
    C_D(App);
    return d->save_settings();
}

auto App::info() const -> const model::AppInfo& {
    C_D(const App);
    return d->info;
}

auto App::app_state() const -> AppState* {
    C_D(const App);
    return d->app_state;
}

void App::register_meta_type() {
    QMetaType::registerConverter<model::ItemId, QString>(&model::ItemId::toString);
}

} // namespace qcm

#include <Qcm/moc_app.cpp>