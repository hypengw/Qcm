#include "qcm_interface/global.h"

#include <thread>
#include <mutex>

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

class Global::Private {
public:
    Private(Global* p)
        : qt_ex(std::make_shared<QtExecutionContext>(p)),
          pool(get_pool_size()),
          session(std::make_shared<request::Session>(pool.get_executor())) {}
    ~Private() {}

    qt_executor_t     qt_ex;
    asio::thread_pool pool;

    rc<request::Session> session;
    //    mutable ncm::Client         m_client;

    std::map<std::string, Client, std::less<>> clients;

    model::AppInfo info;

    std::mutex mutex;

    QList<QObject*> datas;
};

auto Global::instance() -> Global* { return static_global(); }

Global::Global(): d_ptr(make_up<Private>(this)) {
    DEBUG_LOG("init Global");
    _assert_rel_(static_global(this) == this);
}
Global::~Global() {}

auto Global::client(std::string_view name_view, std::optional<std::function<Client()>> init)
    -> Client {
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
void Global::join() {
    C_D(Global);
    d->pool.join();
}

auto Global::datas() -> QQmlListProperty<QObject> {
    C_D(Global);
    return { this, &d->datas };
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
} // namespace qcm