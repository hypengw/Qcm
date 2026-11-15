module;
#include "Qcm/macro.hpp"
#include "core/log.h"

#include "Qcm/app.moc.h"
#undef assert
#include <rstd/macro.hpp>

module qcm;
import :app;
import qcm.activation;
import qcm.log;

using namespace qcm;
using namespace Qt::Literals::StringLiterals;
using rstd::boxed::Box;

namespace
{

auto get_pool_size() -> std::size_t {
    return std::clamp<u32>(std::thread::hardware_concurrency(), 4, 12);
}

auto app_instance(App* in = nullptr) -> App* {
    static App* instance { in };
    rstd_assert(instance != nullptr, "app object not inited");
    rstd_assert(in == nullptr || instance == in, "there should be only one app object");
    return instance;
}

} // namespace

namespace qcm
{

class App::Private {
public:
    Private(App* self, Box<Backend> backend, QString activation_token)
        : m_p(self),
          m_global(make_rc<Global>()),
          m_util(make_rc<qml::Util>()),
          m_playqueu(new qcm::PlayQueue(self)),
          m_empty(new qcm::model::EmptyModel(self)),
          provider_meta_status(new ProviderMetaStatusModel(self)),
          provider_status(new ProviderStatusModel(self)),
          page_model(new PageModel(self)),
          app_state(new AppState(self)),
          m_backend(std::move(backend)),
          m_activation_token(std::move(activation_token)),
          m_main_win(nullptr),
          m_qml_engine(make_up<QQmlApplicationEngine>()) {}
    ~Private() {
        m_qml_engine.reset();

        m_playqueu->drop_global();
        m_global->join();
        // clean query
        qDeleteAll(m_playqueu->children());
    }

    App* m_p;

    rc<Global>    m_global;
    rc<qml::Util> m_util;

    PlayQueue*               m_playqueu;
    model::EmptyModel*       m_empty;
    ProviderMetaStatusModel* provider_meta_status;
    ProviderStatusModel*     provider_status;
    PageModel*               page_model;
    model::AppInfo           info;
    AppState*                app_state;

    Box<Backend> m_backend;
    QString      m_activation_token;

    QPointer<QQuickWindow>           m_main_win;
    Option<Box<QmlNetworkDiskCache>> m_image_cache;
    up<QQmlApplicationEngine>        m_qml_engine;
};

App* App::create(QQmlEngine*, QJSEngine*) {
    auto app = app_instance();
    // not delete by qml
    QJSEngine::setObjectOwnership(app, QJSEngine::CppOwnership);
    return app;
}

App* App::instance() { return app_instance(); }

App::App(Box<Backend> connection, QString activation_token)
    : QObject(nullptr),
      d_ptr(make_box<Private>(this, std::move(connection), std::move(activation_token))) {
    C_D(App);
    app_instance(this);
    register_meta_type();
    register_converters();
    connect_actions();
    connect_components();
    {
        QGuiApplication::setDesktopFileName(APP_ID);
        PageModel::init_main_pages(d->page_model);
    }

    LOG_DEBUG("thread pool size: {}", get_pool_size());
    {
        global()->player()->bindBackend(backend());
        playqueue()->bindBackend(backend());
        connect(backend(), &Backend::playerError, global(), [this](const QString& error) {
            global()->errorOccurred(error);
        });
        connect(global()->player(), &Player::errorOccurred, global(), [this](const QString& error) {
            global()->errorOccurred(error);
        });
        connect(
            backend(), &Backend::playerUiRequested, this, [this](const msg::PlayerUiMsg& request) {
                C_D(App);
                if (request.raise()) {
                    activation::WindowActivation activation(request.activationToken());
                    activation.activate(d->m_main_win);
                }
            });
        {
            auto p = this->backend();
            connect(
                p,
                &Backend::error,
                d->app_state,
                [d, p](QString err) {
                    QObject::disconnect(d->app_state, &AppState::retry, p, &Backend::on_retry);
                    QObject::connect(d->app_state, &AppState::retry, p, &Backend::on_retry);
                    d->app_state->set_state(AppState::Error { err });
                },
                Qt::QueuedConnection);
        }
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
    connect(
        AppStore::instance(), &AppStore::scopeChanged, d->m_playqueu, &PlayQueue::reloadEntities);
    connect(d->provider_status,
            &ProviderStatusModel::syncingChanged,
            d->m_playqueu,
            [queue = d->m_playqueu](bool syncing) {
                if (! syncing) queue->reloadEntities();
            });
    auto engine = this->engine();

    d->m_image_cache = Some(Box<QmlNetworkDiskCache>::make(
        QDir(QString::fromStdString(cache_path().string())).filePath(u"qml-images"_s),
        128 * 1024 * 1024));
    (*d->m_image_cache)->install(engine);

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

    auto update_image_sources = [this, engine] {
        auto backend = this->backend();
        if (! qextra::image::set_local_file_source(
                engine, backend->resource_base_url(), backend->cache_directory()))
            LOG_ERROR("invalid image file source configuration");
        if (! qextra::image::set_network_cache(engine, {}, 300, true))
            LOG_ERROR("invalid image network cache configuration");
    };
    connect(this->backend(), &Backend::resourceStateChanged, engine, update_image_sources);
    connect(AppStore::instance(), &AppStore::scopeChanged, engine, [engine] {
        qextra::image::clear_network_cache(engine);
    });
    update_image_sources();
    activation::WindowActivation activation(std::exchange(d->m_activation_token, {}));
    engine->loadFromModule("Qcm.App", "Window");

    for (auto el : engine->rootObjects()) {
        if (auto win = qobject_cast<QQuickWindow*>(el)) {
            d->m_main_win = win;
        }
    }

    rstd_assert(! d->m_main_win.isNull(), "main window must exist");
    activation.activate(d->m_main_win);
    backend()->start_delivery();
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

QVariant App::import_path_list() {
    C_D(App);
    return d->m_qml_engine->importPathList();
}

void App::test() {}

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
    return d->m_backend.as_mut_ptr();
}
auto App::util() const -> qml::Util* {
    C_D(const App);
    return d->m_util.get();
}
auto App::playqueue() const -> PlayQueue* {
    C_D(const App);
    return d->m_playqueu;
}
// #include <private/qquickpixmapcache_p.h>
void App::releaseResources(QQuickWindow*, const QJSValue& extra) {
    C_D(App);
    LOG_INFO("gc");
    // win->releaseResources();
    d->m_qml_engine->trimComponentCache();
    d->m_qml_engine->collectGarbage();
    // QQuickPixmap::purgeCache();
    plt::malloc_trim(0);
    auto as_mb = [](usize n) {
        return std::format("{:.2f} MB", n / (1024.0 * 1024.0));
    };

    auto print_mem_stat = [as_mb](MemoryStats* s) {
        return std::format("used({}): {}, peak: {}",
                           s->current_block_count(),
                           as_mb(s->current_bytes()),
                           as_mb(s->peak_bytes()));
    };

    auto store = AppStore::instance();

    LOG_DEBUG("entity store: {} entries, {} bytes", store->size(), store->bytes());

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
store:
  {}
)",
              as_mb(info.heap),
              info.mmap_num,
              as_mb(info.mmap),
              as_mb(info.totle_in_use),
              extra.property("pool_obj").toInt(),
              qr_image_response_count().load(),
              print_mem_stat(mem_mgr().pool_stat),
              print_mem_stat(mem_mgr().session_mem),
              print_mem_stat(mem_mgr().backend_mem),
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
auto App::info() const -> const model::AppInfo& {
    C_D(const App);
    return d->info;
}

auto App::app_state() const -> AppState* {
    C_D(const App);
    return d->app_state;
}

void App::register_meta_type() { model::registerIdConverters(); }

void App::connect_components() {
    connect(playqueue(), &PlayQueue::changingTrackChanged, global()->player(), [this] {
        global()->player()->setChangingTrack(playqueue()->changingTrack());
    });
    connect(provider_status(),
            &ProviderStatusModel::libraryStatusChanged,
            this,
            &App::libraryStatusChanged);
}

void App::connect_actions() {
    connect(Action::instance(), &Action::queue, this, &App::onQueue);
    connect(Action::instance(), &Action::queue_next, this, &App::onQueueNext);
    connect(Action::instance(), &Action::switch_songs, this, &App::onSwitch);
    connect(Action::instance(), &Action::play, this, &App::onPlay);
    connect(Action::instance(), &Action::switch_queue, this, &App::onSwitchQueue);
    connect(Action::instance(), &Action::routeItem, this, &App::onRouteItem);
    connect(Action::instance(), &Action::next, playqueue(), &PlayQueue::next);
    connect(Action::instance(), &Action::prev, playqueue(), &PlayQueue::prev);
    connect(Action::instance(), &Action::toggle, global()->player(), &Player::toggle);
    connect(Action::instance(),
            &Action::playUrl,
            global()->player(),
            [player = global()->player()](const QUrl& url, bool) {
                player->set_source(url);
            });
    connect(Action::instance(), &Action::scheduleRenderJob, qApp, [](QRunnable* job, qint32 stage) {
        if (QWindowList wins = qApp->allWindows(); ! wins.isEmpty()) {
            if (auto win = qobject_cast<QQuickWindow*>(wins.front())) {
                win->scheduleRenderJob(job, (QQuickWindow::RenderStage)stage);
            }
        }
    });
}

void App::onPlay(model::ItemId song, model::ItemId) { playqueue()->playSong(song); }
void App::onQueueNext(const std::vector<model::ItemId>& songs, model::ItemId) {
    playqueue()->insertNext(songs);
}
void App::onQueue(const std::vector<model::ItemId>& songs, model::ItemId) {
    playqueue()->appendSongs(songs);
}
void App::onSwitch(const std::vector<model::ItemId>& songs, model::ItemId) {
    playqueue()->replaceSongs(songs);
}
void App::onSwitchQueue(model::DynamicIdQueue* queue) {
    if (queue) queue->activate();
}

auto App::itemRouteProperties(const model::ItemId& id, const QVariantMap& in_props) const
    -> QVariantMap {
    auto props = in_props;
    props.remove("itemId");
    props.remove("artistId");
    props.remove("albumId");
    props.remove("mixId");
    if (auto artist = id.toArtistId()) {
        props["artistId"] = QVariant::fromValue(*artist);
        if (! props.contains("albumArtist"))
            props["albumArtist"] = id.type() == enums::ItemType::ItemAlbumArtist;
    } else if (auto album = id.toAlbumId()) {
        props["albumId"] = QVariant::fromValue(*album);
    } else if (auto mix = id.toMixId()) {
        props["mixId"] = QVariant::fromValue(*mix);
    } else {
        props["itemId"] = QVariant::fromValue(id);
    }
    return props;
}

void App::onRouteItem(const model::ItemId& id, const QVariantMap& in_props) {
    const auto url = id.toPageUrl();
    if (! id.valid() || url.isEmpty()) return;

    Action::instance()->route(enums::SpecialRoute::SRMain);

    model::RouteMsg msg;

    msg.dst   = url.toString();
    msg.props = itemRouteProperties(id, in_props);

    Action::instance()->route(QVariant::fromValue(msg));
}

} // namespace qcm

#include "Qcm/app.moc.cpp"
