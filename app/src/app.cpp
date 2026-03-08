module;
#include "Qcm/macro.hpp"
#include "core/log.h"
#include "mpris/mpris.h"
#include "core/sender.h"
#include "player/player.h"

#include "Qcm/app.moc.h"
#undef assert
#include <rstd/macro.hpp>

module qcm;
import :app;
import qcm.log;


using namespace qcm;
using namespace Qt::Literals::StringLiterals;

DEFINE_CONVERT(ncrequest::req_opt::Proxy::Type, enums::ProxyType) {
    out = static_cast<rstd::mtp::decay_t<decltype(out)>>(in);
}

namespace
{

void cache_clean_cb(const cppstd::filesystem::path& cache_dir, std::string_view key) {
    cppstd::error_code ec;
    auto               file = cache_dir / (key.size() >= 2 ? key.substr(0, 2) : "no"sv) / key;
    cppstd::filesystem::remove(file, ec);
    LOG_DEBUG("cache remove {}", file.string());
}

auto get_pool_size() -> std::size_t {
    return std::clamp<u32>(cppstd::thread::hardware_concurrency(), 4, 12);
}

auto app_instance(App* in = nullptr) -> App* {
    static App* instance { in };
    assert(instance != nullptr, "app object not inited");
    assert(in == nullptr || instance == in, "there should be only one app object");
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
          m_mpris(Box<mpris::Mpris>::make()),
#endif
          m_main_win(nullptr),
          m_qml_engine(Box<QQmlApplicationEngine>::make()) {
    }
    ~Private() {
        m_qml_engine.reset();

        save_settings();
        m_playqueu->drop_global();
        m_global->join();
    }

    void save_settings() {
        QSettings s;
        s.setValue("play/loop", (int)m_playqueu->loopMode());
    }

    App* m_p;

    rc<Global>    m_global;
    rc<qml::Util> m_util;

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

    Option<Box<Backend>> m_backend;

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
        d->m_backend = Some(Box<Backend>::make(d->m_global->session()));
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

        auto data_dir  = convert_from<QString>(data_path().string());
        auto cache_dir = convert_from<QString>(cache_path().string());
        this->backend()->start(backend_exe, data_dir, cache_dir);
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

    assert(d->m_main_win, "main window must exist");
    assert(d->m_player_sender, "player must init");
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
    return d->m_qml_engine.as_mut_ptr();
}
auto App::global() const -> Global* {
    C_D(const App);
    return d->m_global.get();
}
auto App::backend() const -> Backend* {
    C_D(const App);
    return (*(d->m_backend)).as_mut_ptr();
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

void App::connect_components() {
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
    connect(Action::instance(), &Action::record, this, &App::onRecord);
    connect(Action::instance(), &Action::routeItem, this, &App::onRouteItem);

    connect(
        Action::instance(), &Action::next, this->playqueue(), QOverload<>::of(&PlayQueue::next));
    connect(
        Action::instance(), &Action::prev, this->playqueue(), QOverload<>::of(&PlayQueue::prev));

    // player
    {
        const auto player = global()->player();
        connect(Action::instance(),
                &Action::playUrl,
                player,
                [p = player](const QUrl& url, bool reload) {
                    if (reload && p->source() == url) {
                        p->reset_source();
                    }
                    p->set_source(url);

                    if (url.isValid()) p->play();
                });

        connect(Action::instance(), &Action::toggle, player, &Player::toggle);

        connect(
            player,
            &Player::playbackStateChanged,
            player,
            [p = player](Player::PlaybackState, Player::PlaybackState new_) {
                auto queue = App::instance()->playqueue();
                if (new_ == Player::PlaybackState::StoppedState && p->source().isValid()) {
                    if (p->duration() > 0 && p->position() / (double)p->duration() > 0.98) {
                        queue->next(queue->loopMode());
                    }
                }

                if (new_ == Player::PlaybackState::PlayingState) {
                    auto cur = queue->currentId().unwrap_or(model::ItemId {});
                    Action::instance()->playLog(
                        (qint32)msg::model::PlaylogActionGadget::PlaylogAction::PLAYLOG_ACTION_PLAY,
                        cur,
                        {});
                }
            });
    }

    // stop logger
    connect(
        Action::instance(),
        &Action::record,
        this,
        [this,
         old_id        = rstd::Option<model::ItemId> {},
         old_source_id = rstd::Option<model::ItemId>()](enums::RecordAction act) mutable {
            switch (act) {
            case enums::RecordAction::RecordSwitch:
            case enums::RecordAction::RecordNext:
            case enums::RecordAction::RecordPrev: {
                if (old_id) {
                    Action::instance()->playLog(
                        (qint32)msg::model::PlaylogActionGadget::PlaylogAction::PLAYLOG_ACTION_STOP,
                        *old_id,
                        old_source_id.unwrap_or(model::ItemId {}));
                }
                old_id = this->playqueue()->currentId();
                auto source_id_var =
                    this->playqueue()->currentData(this->playqueue()->roleOf("sourceId"));
                if (auto source_id_p = get_if<model::ItemId>(&source_id_var)) {
                    old_source_id = rstd::Some(*source_id_p);
                } else {
                    old_source_id = rstd::None();
                }
                break;
            }
            default: {
            }
            }
        });

    {
        using namespace std::chrono;
        struct Timer {
            time_point<system_clock> point;
            milliseconds             passed;
        };

        connect(Action::instance(),
                &Action::playLog,
                this,
                [this, t = make_rc<Timer>()](
                    qint32 act_int, model::ItemId song_id, model::ItemId source) {
                    using PlaylogAction = msg::model::PlaylogActionGadget::PlaylogAction;
                    const auto act      = (PlaylogAction)act_int;
                    auto       backend  = this->backend();
                    auto       req      = msg::PlaylogReq {};
                    const auto now      = system_clock::now();

                    req.setSongId(song_id.id());
                    req.setAction(act);
                    req.setTimestamp(duration_cast<milliseconds>(now.time_since_epoch()).count());
                    req.setSourceId(source.id());
                    req.setSourceType((msg::model::ItemTypeGadget::ItemType)source.type());

                    asio::co_spawn(
                        qcm::strand_executor(),
                        [backend, req] mutable -> task<void> {
                            co_await backend->send(std::move(req));
                        },
                        asio_detached_log_t {});
                });
    }

    connect(Action::instance(), &Action::record, this, [this](enums::RecordAction act) {
        switch (act) {
        case enums::RecordAction::RecordSwitchQueue:
        case enums::RecordAction::RecordSwitch:
        case enums::RecordAction::RecordNext:
        case enums::RecordAction::RecordPrev: {
            break;
        }
        default: {
            return;
        }
        }

        auto curId = playqueue()->currentId();
        if (! curId || ! curId->valid()) return;
        auto b = this->backend();
        Action::instance()->playUrl(b->audio_url(*curId), true);
    });

    connect(Action::instance(), &Action::scheduleRenderJob, qApp, [](QRunnable* job, qint32 stage) {
        if (QWindowList wins = qApp->allWindows(); ! wins.isEmpty()) {
            if (auto win = qobject_cast<QQuickWindow*>(wins.front())) {
                win->scheduleRenderJob(job, (QQuickWindow::RenderStage)stage);
            }
        }
    });
}

void App::onPlay(model::ItemId songId, model::ItemId sourceId) {
    switchPlayIdQueue();

    auto q   = App::instance()->play_id_queue();
    auto row = q->rowCount();
    q->insert(row, std::array { songId });
    if (sourceId.valid())
        App::instance()->playqueue()->updateSourceId(std::array { songId }, sourceId);
    q->setCurrentIndex(songId);
    Action::instance()->record(enums::RecordAction::RecordSwitch);
}

void App::onQueueNext(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
    switchPlayIdQueue();

    auto q        = App::instance()->play_id_queue();
    auto idx      = std::min(q->currentIndex() + 1, q->rowCount());
    auto inserted = q->insert(idx, songIds);
    {
        auto q = App::instance()->playqueue();
        if (sourceId.valid()) q->updateSourceId(songIds, sourceId);
        q->startIfNoCurrent();
    }
    Action::instance()->toast(QString::fromStdString(
        inserted > 0 ? std::format("Add {} songs to queue", inserted) : "Already added"s));
}
void App::onQueue(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
    switchPlayIdQueue();

    auto q        = App::instance()->play_id_queue();
    auto inserted = q->insert(q->rowCount(), songIds);
    {
        auto q = App::instance()->playqueue();
        if (sourceId.valid()) q->updateSourceId(songIds, sourceId);
        q->startIfNoCurrent();
    }
    Action::instance()->toast(QString::fromStdString(
        inserted > 0 ? std::format("Add {} songs to queue", inserted) : "Already added"s));
}
void App::onSwitch(const std::vector<model::ItemId>& songIds, model::ItemId sourceId) {
    switchPlayIdQueue();

    auto q = App::instance()->play_id_queue();
    q->removeRows(0, q->rowCount());
    q->insert(q->rowCount(), songIds);
    {
        auto q = App::instance()->playqueue();
        if (sourceId.valid()) q->updateSourceId(songIds, sourceId);
        q->startIfNoCurrent();
    }
}

void App::onRecord(enums::RecordAction) {}

void App::onSwitchQueue(model::IdQueue* queue) {
    if (queue == nullptr) {
        LOG_INFO("queue is null");
    } else {
        this->playqueue()->setSourceModel(queue);
        if (queue->rowCount()) {
            queue->setCurrentIndex(0);
        }
        if (queue->rowCount() <= 1) {
            queue->requestNext();
        }
        Action::instance()->record(enums::RecordAction::RecordSwitchQueue);
    }
}

void App::onRouteItem(const model::ItemId& id, const QVariantMap& in_props) {
    Action::instance()->route(enums::SpecialRoute::SRMain);

    model::RouteMsg msg;
    auto            url = id.toPageUrl();

    auto props      = in_props;
    msg.dst         = url.toString();
    props["itemId"] = QVariant::fromValue(id);
    msg.props       = std::move(props);

    Action::instance()->route(QVariant::fromValue(msg));
}

} // namespace qcm

#include "Qcm/app.moc.cpp"