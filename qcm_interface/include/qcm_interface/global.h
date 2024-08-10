#pragma once

#include <QtCore/QObject>
#include <QtQml/QQmlListProperty>
#include <any>

#include <asio/thread_pool.hpp>
#include <asio/steady_timer.hpp>

#include "core/core.h"
#include "asio_qt/qt_executor.h"
#include "qcm_interface/export.h"

#include "qcm_interface/model/app_info.h"

namespace request
{
class Session;
}
namespace qcm
{

class GlobalWrapper;
class QCM_INTERFACE_API Global : public QObject {
    Q_OBJECT
    Q_PROPERTY(model::AppInfo info READ info CONSTANT FINAL)

    friend class GlobalWrapper;

public:
    using pool_executor_t = asio::thread_pool::executor_type;
    using qt_executor_t   = QtExecutor;

    static auto instance() -> Global*;
    static void setInstance(Global*);

    Global();
    ~Global();

    struct Client {
        struct Api {
            auto (*server_url)(std::any&, const model::ItemId&) -> std::string;
        };
        rc<Api>  api;
        std::any instance;
    };

    auto client(std::string_view name, std::optional<std::function<Client()>> = std::nullopt)
        -> Client;

    auto qexecutor() -> qt_executor_t&;
    auto pool_executor() -> pool_executor_t;
    auto session() -> rc<request::Session>;

    auto info() const -> const model::AppInfo&;

    void join();

    Q_INVOKABLE QVariant server_url(const model::ItemId&);

Q_SIGNALS:
    void errorOccurred(QString);
    void toast(QString text, qint32 duration);

private:
    class Private;
    C_DECLARE_PRIVATE(Global, d_ptr);
};

class QCM_INTERFACE_API GlobalWrapper : public QObject {
    Q_OBJECT
    Q_CLASSINFO("DefaultProperty", "datas")
    QML_ELEMENT

    Q_PROPERTY(QQmlListProperty<QObject> datas READ datas FINAL)
    Q_PROPERTY(model::AppInfo info READ info CONSTANT FINAL)
public:
    GlobalWrapper();
    ~GlobalWrapper();

    auto                 datas() -> QQmlListProperty<QObject>;
    auto                 info() -> const model::AppInfo&;
    Q_INVOKABLE QVariant server_url(const model::ItemId&);

Q_SIGNALS:
    void errorOccurred(QString, bool from_global = false);
    void toast(QString text, qint32 duration = 4000, bool from_global = false);

private:
    template<typename R, typename... ARGS, typename... ARGS2>
    void connect_from_global(Global* g, R (Global::*g_func)(ARGS...),
                             R (GlobalWrapper::*gw_func)(ARGS2...)) {
        QObject::connect(g, g_func, this, [this, gw_func](ARGS... args) {
            (this->*gw_func)(args..., true);
        });
    }

    template<typename R, typename... ARGS, typename... ARGS2>
    void connect_to_global(Global* g, R (Global::*g_func)(ARGS...),
                           R (GlobalWrapper::*gw_func)(ARGS2...)) {
        QObject::connect(this, gw_func, g, [g, g_func](ARGS... args, bool from_global) {
            if (! from_global) {
                (g->*g_func)(args...);
            }
        });
    }

    Global*         m_g;
    QList<QObject*> m_datas;
};

} // namespace qcm