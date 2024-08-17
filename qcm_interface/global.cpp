#include "qcm_interface/global.h"

#include <thread>
#include <mutex>

#include <QUuid>

#include "core/log.h"
#include "request/session.h"

namespace
{

auto get_pool_size() -> std::size_t {
    return std::clamp<u32>(std::thread::hardware_concurrency(), 4, 12);
}

auto static_global(qcm::Global* set = nullptr) -> qcm::Global* {
    static qcm::Global* theGlobal { set };
    _assert_rel_(theGlobal);
    return theGlobal;
}
} // namespace

namespace qcm
{
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

class Global::Private {
public:
    Private(Global* p)
        : qt_ex(std::make_shared<QtExecutionContext>(p, (QEvent::Type)QEvent::registerEventType())),
          pool(get_pool_size()),
          session(std::make_shared<request::Session>(pool.get_executor())),
          copy_action_comp(nullptr) {}
    ~Private() {}

    qt_executor_t     qt_ex;
    asio::thread_pool pool;

    rc<request::Session> session;
    //    mutable ncm::Client         m_client;

    QUuid                     uuid;
    rc<media_cache::DataBase> cache_sql;

    std::map<std::string, Client, std::less<>> clients;

    model::AppInfo info;
    QQmlComponent* copy_action_comp;

    std::mutex mutex;
};

auto Global::instance() -> Global* { return static_global(); }

Global::Global(): d_ptr(make_up<Private>(this)) {
    DEBUG_LOG("init Global");
    _assert_rel_(static_global(this) == this);
}
Global::~Global() {}

auto Global::client(std::string_view                       name_view,
                    std::optional<std::function<Client()>> init) -> Client {
    C_D(Global);
    std::unique_lock l { d->mutex };
    auto             name = std::string(name_view);
    if (! d->clients.contains(name_view)) {
        if (init) {
            d->clients.insert({ name, init.value()() });
            return d->clients.at(name);
        } else {
            return {};
        }
    } else {
        return d->clients.at(name);
    }
}
auto Global::info() const -> const model::AppInfo& {
    C_D(const Global);
    return d->info;
}

auto Global::qexecutor() -> qt_executor_t& {
    C_D(Global);
    return d->qt_ex;
}
auto Global::pool_executor() -> pool_executor_t {
    C_D(Global);
    return d->pool.executor();
}
auto Global::session() -> rc<request::Session> {
    C_D(Global);
    return d->session;
}
auto Global::uuid() const -> const QUuid& {
    C_D(const Global);
    return d->uuid;
}
auto Global::get_cache_sql() const -> rc<media_cache::DataBase> {
    C_D(const Global);
    return d->cache_sql;
}
auto Global::copy_action_comp() const -> QQmlComponent* {
    C_D(const Global);
    return d->copy_action_comp;
}
void Global::set_copy_action_comp(QQmlComponent* val) {
    C_D(Global);
    if (std::exchange(d->copy_action_comp, val) != val) {
        copyActionCompChanged();
    }
}
void Global::set_uuid(const QUuid& val) {
    C_D(Global);
    if (std::exchange(d->uuid, val) != val) {
        uuidChanged();
    }
}
void Global::set_cache_sql(rc<media_cache::DataBase> val) {
    C_D(Global);
    d->cache_sql = val;
}
void Global::join() {
    C_D(Global);
    d->pool.join();
}

auto Global::server_url(const model::ItemId& id) -> QVariant {
    C_D(Global);
    const auto& p = id.provider().toStdString();
    if (d->clients.contains(p)) {
        auto& c = d->clients.at(p);
        return convert_from<QString>(c.api->server_url(c.instance, id));
    }
    return {};
}

GlobalWrapper::GlobalWrapper(): m_g(Global::instance()) {
    connect_from_global(m_g, &Global::copyActionCompChanged, &GlobalWrapper::copyActionCompChanged);
    connect_from_global(m_g, &Global::errorOccurred, &GlobalWrapper::errorOccurred);
    connect_from_global(m_g, &Global::toast, &GlobalWrapper::toast);
    connect_from_global(m_g, &Global::uuidChanged, &GlobalWrapper::uuidChanged);

    connect_to_global(m_g, &Global::errorOccurred, &GlobalWrapper::errorOccurred);
    connect_to_global(m_g, &Global::toast, &GlobalWrapper::toast);

    connect(this, &GlobalWrapper::errorOccurred, this, [this](const QString& error) {
        if (! error.endsWith("Operation aborted.")) {
            QObject* act { nullptr };
            auto     comp = copy_action_comp();
            if (comp) {
                QVariantMap map;
                map.insert("error", error);
                act = comp->createWithInitialProperties(map);
            }
            toast(error, 0, enums::ToastFlag::TFCloseable, act);
        }
    });
}
GlobalWrapper::~GlobalWrapper() {}
auto GlobalWrapper::datas() -> QQmlListProperty<QObject> { return { this, &m_datas }; }
auto GlobalWrapper::info() -> const model::AppInfo& { return m_g->info(); }
auto GlobalWrapper::server_url(const model::ItemId& id) -> QVariant { return m_g->server_url(id); }
auto GlobalWrapper::copy_action_comp() const -> QQmlComponent* { return m_g->copy_action_comp(); }
auto GlobalWrapper::uuid() const -> QString { return m_g->uuid().toString(QUuid::WithoutBraces); }
void GlobalWrapper::set_copy_action_comp(QQmlComponent* val) { m_g->set_copy_action_comp(val); }

} // namespace qcm