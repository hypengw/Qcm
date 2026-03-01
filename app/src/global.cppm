module;
#include <thread>
#include <filesystem>
#include <mutex>
#include <QUuid>
#include <QPluginLoader>
#include <QCoreApplication>
#include <QtQml/QQmlListProperty>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>

#include <asio/bind_executor.hpp>
#include <asio/thread_pool.hpp>
#include <asio/steady_timer.hpp>
#include <asio/thread_pool.hpp>
#include <asio/strand.hpp>

#include "core/asio/detached_log.h"

#include "Qcm/global.moc.h"
#ifdef Q_MOC_RUN
#    include "Qcm/global.moc"
#endif

#include "core/asio/task.h"
#include "core/qasio/qt_executor.h"

#include "player/metadata.h"
#include "Qcm/status/app_state.hpp"

#include "core/log.h"
#include "Qcm/util/path.hpp"

export module qcm.global;
export import qcm.action;
export import qcm.qml.enums;
export import qcm.util.mem;
export import qcm.util.global_static;
export import qcm.model.app_info;
export import qcm.player;
import ncrequest;

namespace qcm
{
class PluginModel;
class QcmPluginInterface;
class UserModel;

namespace db
{
class ItemSqlBase;
}

struct StopSignal {
    bool val { false };
};

export class GlobalWrapper;
export class Global : public QObject {
    Q_OBJECT

    friend class GlobalWrapper;
    friend class PluginModel;

public:
    using pool_executor_t = asio::thread_pool::executor_type;
    using qt_executor_t   = QtExecutor;
    using Metadata        = player::Metadata;

    static auto instance() -> Global*;

    Global();
    ~Global();

    auto qexecutor() -> qt_executor_t&;
    auto pool_executor() -> pool_executor_t;
    auto session() -> rc<ncrequest::Session>;

    auto uuid() const -> const QUuid&;
    auto player() const -> Player*;

    auto get_metadata(const std::filesystem::path&) const -> Metadata;

    void join();

    Q_SIGNAL void errorOccurred(QString error, StopSignal stop = {});
    Q_SIGNAL void copyActionCompChanged(StopSignal stop = {});
    Q_SIGNAL void uuidChanged(StopSignal stop = {});
    Q_SIGNAL void sessionChanged(StopSignal stop = {});

    using MetadataImpl = std::function<Metadata(const std::filesystem::path&)>;
    void        set_uuid(const QUuid&);
    void        set_metadata_impl(const MetadataImpl&);
    static void setInstance(Global*);

private:
    class Private;
    C_DECLARE_PRIVATE(Global, d_ptr);
};

class GlobalWrapper : public QObject {
    Q_OBJECT
    Q_CLASSINFO("DefaultProperty", "datas")
    QML_ELEMENT

    Q_PROPERTY(QQmlListProperty<QObject> datas READ datas FINAL)
    Q_PROPERTY(QQmlComponent* toastActionComp READ toastActionComp WRITE setToastActionComp NOTIFY
                   toastCompActionChanged FINAL)
    Q_PROPERTY(QString uuid READ uuid NOTIFY uuidChanged FINAL)
    Q_PROPERTY(qcm::Player* player READ player CONSTANT FINAL)
public:
    GlobalWrapper();
    ~GlobalWrapper();

    auto datas() -> QQmlListProperty<QObject>;
    auto uuid() const -> QString;

    auto toastActionComp() const noexcept -> QQmlComponent*;
    void setToastActionComp(QQmlComponent* val);

    auto player() const -> Player*;

    Q_SIGNAL void toastCompActionChanged();

    // forward singals
    Q_SIGNAL void errorOccurred(QString error, StopSignal stop = {});
    Q_SIGNAL void copyActionCompChanged(StopSignal stop = {});
    Q_SIGNAL void uuidChanged(StopSignal stop = {});
    Q_SIGNAL void sessionChanged(StopSignal stop = {});

private:
    template<typename R, typename... ARGS>
    void connect_from_global(Global* g, R (Global::*g_func)(ARGS...),
                             R (GlobalWrapper::*gw_func)(ARGS...));

    template<typename R, typename... ARGS>
    void connect_to_global(Global* g, R (Global::*g_func)(ARGS...),
                           R (GlobalWrapper::*gw_func)(ARGS...));

    Global*         m_g;
    QList<QObject*> m_datas;
    QQmlComponent*  m_toast_action_comp;
};

export auto mem_mgr() -> MemResourceMgr&;
export auto qexecutor_switch() -> task<void>;
export auto qexecutor() -> QtExecutor&;
export auto pool_executor() -> asio::thread_pool::executor_type;
export auto strand_executor() -> asio::strand<asio::thread_pool::executor_type>;
} // namespace qcm

namespace qcm
{

auto get_pool_size() -> std::size_t;

class Global::Private {
public:
    Private(Global* p);
    ~Private();

    Arc<QtExecutionContext> qt_ctx;
    asio::thread_pool       pool;

    Arc<ncrequest::Session> session;

    QUuid uuid;

    MetadataImpl metadata_impl;

    Player* player;

    mutable std::mutex mutex;
};
} // namespace qcm

module :private;

namespace
{

auto static_global(qcm::Global* set = nullptr) -> qcm::Global* {
    static qcm::Global* theGlobal { set };
    _assert_rel_(theGlobal);
    return theGlobal;
}

} // namespace

auto qcm::qexecutor() -> QtExecutor& { return Global::instance()->qexecutor(); }
auto qcm::qexecutor_switch() -> task<void> {
    return asio::post(asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
}
auto qcm::pool_executor() -> asio::thread_pool::executor_type {
    return Global::instance()->pool_executor();
}
auto qcm::strand_executor() -> asio::strand<asio::thread_pool::executor_type> {
    return asio::make_strand(Global::instance()->pool_executor());
}
auto qcm::mem_mgr() -> MemResourceMgr& {
    static MemResourceMgr the_mgr {};
    return the_mgr;
}

namespace qcm
{

auto get_pool_size() -> std::size_t {
    return std::clamp<u32>(std::thread::hardware_concurrency(), 4, 8);
}

namespace
{
template<typename T>
bool get_stop_signal(T&& t) {
    if constexpr (std::same_as<std::remove_cvref_t<T>, StopSignal>) {
        return t.val;
    } else {
        return false;
    }
}
template<typename T>
auto hook_stop_signal(T&& t) {
    if constexpr (std::same_as<std::remove_cvref_t<T>, StopSignal>) {
        return StopSignal { true };
    } else {
        return std::forward<T>(t);
    }
}

} // namespace
template<typename R, typename... ARGS>
void GlobalWrapper::connect_from_global(Global* g, R (Global::*g_func)(ARGS...),
                                        R (GlobalWrapper::*gw_func)(ARGS...)) {
    QObject::connect(g, g_func, this, [this, gw_func](ARGS... args) {
        auto stop = (get_stop_signal(args) || ...);
        if (! stop) {
            (this->*gw_func)(hook_stop_signal(args)...);
        }
    });
}
template<typename R, typename... ARGS>
void GlobalWrapper::connect_to_global(Global* g, R (Global::*g_func)(ARGS...),
                                      R (GlobalWrapper::*gw_func)(ARGS...)) {
    QObject::connect(this, gw_func, g, [g, g_func](ARGS... args) {
        auto stop = (get_stop_signal(args) || ...);
        if (! stop) {
            (g->*g_func)(hook_stop_signal(args)...);
        }
    });
}

Global::Private::Private(Global* p)
    : qt_ctx(make_arc<QtExecutionContext>(p, (QEvent::Type)QEvent::registerEventType())),
      pool(get_pool_size()),
      session(ncrequest::Session::make(pool.get_executor(), mem_mgr().session_mem)),
      player(nullptr) {}
Global::Private::~Private() {}

auto Global::instance() -> Global* { return static_global(); }

Global::Global(): d_ptr(make_up<Private>(this)) {
    C_D(Global);
    LOG_DEBUG("init Global");
    _assert_rel_(static_global(this) == this);

    {
        QCoreApplication::setApplicationName(APP_NAME);
        QCoreApplication::setOrganizationName(APP_NAME);
    }

    d->player = new Player(qcm::pool_executor(), &mem_mgr(), this);
    asio::co_spawn(
        asio::strand<Player::channel_type::executor_type>(qcm::pool_executor()),
        [player = d->player]() -> asio::awaitable<void> {
            co_await player->process_msg();
            co_return;
        },
        helper::asio_detached_log_t {});
}
Global::~Global() {
    // delete child before private pointer
    qDeleteAll(children());
    GlobalStatic::instance()->reset();
}
auto Global::qexecutor() -> qt_executor_t& {
    C_D(Global);
    return d->qt_ctx->get_executor();
}
auto Global::pool_executor() -> pool_executor_t {
    C_D(Global);
    return d->pool.executor();
}
auto Global::session() -> rc<ncrequest::Session> {
    C_D(Global);
    return d->session;
}

auto Global::uuid() const -> const QUuid& {
    C_D(const Global);
    return d->uuid;
}

auto Global::player() const -> Player* {
    C_D(const Global);
    return d->player;
}

auto Global::get_metadata(const std::filesystem::path& path) const -> Metadata {
    C_D(const Global);
    if (d->metadata_impl) {
        return d->metadata_impl(path);
    }
    return {};
}

void Global::set_uuid(const QUuid& val) {
    C_D(Global);
    if (ycore::cmp_set(d->uuid, val)) {
        uuidChanged();
    }
}
void Global::set_metadata_impl(const MetadataImpl& impl) {
    C_D(Global);
    d->metadata_impl = impl;
}

void Global::join() {
    C_D(Global);
    session()->about_to_stop();
    player()->close();

    delete d->player;
    d->player = nullptr;

    d->pool.join();
}

GlobalWrapper::GlobalWrapper(): m_g(Global::instance()), m_toast_action_comp(nullptr) {
    connect_from_global(m_g, &Global::copyActionCompChanged, &GlobalWrapper::copyActionCompChanged);
    connect_from_global(m_g, &Global::errorOccurred, &GlobalWrapper::errorOccurred);
    connect_from_global(m_g, &Global::uuidChanged, &GlobalWrapper::uuidChanged);

    connect_to_global(m_g, &Global::errorOccurred, &GlobalWrapper::errorOccurred);

    connect(this, &GlobalWrapper::errorOccurred, this, [this](const QString& error) {
        if (! error.endsWith("Operation aborted.")) {
            QObject* act { nullptr };
            auto     comp = toastActionComp();
            if (comp) {
                QVariantMap map;
                map.insert("error", error);
                act = comp->createWithInitialProperties(map);
            }
            Action::instance()->toast(error, 0, enums::ToastFlag::TFCloseable, act);
        }
    });
}
GlobalWrapper::~GlobalWrapper() {}
auto GlobalWrapper::datas() -> QQmlListProperty<QObject> { return { this, &m_datas }; }
auto GlobalWrapper::uuid() const -> QString { return m_g->uuid().toString(QUuid::WithoutBraces); }
auto GlobalWrapper::toastActionComp() const noexcept -> QQmlComponent* {
    return m_toast_action_comp;
}
void GlobalWrapper::setToastActionComp(QQmlComponent* val) {
    if (m_toast_action_comp != val) {
        m_toast_action_comp = val;
        toastCompActionChanged();
    }
}
auto GlobalWrapper::player() const -> Player* { return m_g->player(); }

} // namespace qcm

#include "Qcm/global.moc.cpp"