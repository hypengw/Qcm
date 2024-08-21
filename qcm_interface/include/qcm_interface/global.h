#pragma once

#include <any>
#include <filesystem>

#include <QtCore/QObject>
#include <QtQml/QQmlListProperty>
#include <QtQml/QQmlComponent>

#include <asio/thread_pool.hpp>
#include <asio/steady_timer.hpp>

#include "core/core.h"
#include "asio_qt/qt_executor.h"
#include "qcm_interface/export.h"
#include "qcm_interface/model/app_info.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/cache_sql.h"
#include "qcm_interface/metadata.h"

namespace request
{
class Session;
}
namespace qcm
{
class App;

struct StopSignal {
    bool val { false };
};

class GlobalWrapper;
class QCM_INTERFACE_API Global : public QObject {
    Q_OBJECT
    Q_PROPERTY(model::AppInfo info READ info CONSTANT FINAL)
    Q_PROPERTY(QQmlComponent* copy_action_comp READ copy_action_comp WRITE set_copy_action_comp
                   NOTIFY copyActionCompChanged FINAL)
    friend class GlobalWrapper;
    friend class App;

public:
    using pool_executor_t = asio::thread_pool::executor_type;
    using qt_executor_t   = QtExecutor;
    using Metadata        = player::Metadata;

    static auto instance() -> Global*;

    Global();
    ~Global();

    struct Client {
        struct Api {
            auto (*server_url)(std::any&, const model::ItemId&) -> std::string;
            auto (*image_cache)(std::any&, const QUrl& url, QSize req) -> std::filesystem::path;
            void (*play_state)(std::any&, enums::PlaybackState state, model::ItemId item,
                               model::ItemId source, i64 played_second, QVariantMap extra);
        };

        operator bool() const { return instance.has_value(); }

        rc<Api>  api;
        std::any instance;
    };

    auto client(std::string_view name,
                std::optional<std::function<Client()>> = std::nullopt) -> Client;

    auto qexecutor() -> qt_executor_t&;
    auto pool_executor() -> pool_executor_t;
    auto session() -> rc<request::Session>;
    auto get_cache_sql() const -> rc<media_cache::DataBase>;

    auto copy_action_comp() const -> QQmlComponent*;
    auto uuid() const -> const QUuid&;

    auto info() const -> const model::AppInfo&;

    auto get_metadata(const std::filesystem::path&) const -> Metadata;

    void join();

    Q_INVOKABLE QVariant server_url(const model::ItemId&);

Q_SIGNALS:
    void errorOccurred(QString error, StopSignal stop = {});
    void toast(QString text, qint32 duration = 4000, enums::ToastFlags = {},
               QObject* action = nullptr, StopSignal stop = {});
    void copyActionCompChanged(StopSignal stop = {});
    void uuidChanged(StopSignal stop = {});
    void playbackLog(enums::PlaybackState state, model::ItemId item, model::ItemId souce,
                     QVariantMap extra = {}, StopSignal stop = {});

public Q_SLOTS:
    void set_copy_action_comp(QQmlComponent*);

private:
    using MetadataImpl = std::function<Metadata(const std::filesystem::path&)>;
    void set_uuid(const QUuid&);
    void set_cache_sql(rc<media_cache::DataBase>);
    void set_metadata_impl(const MetadataImpl&);
    auto get_client(std::string_view) -> Client*;

    static void setInstance(Global*);

    class Private;
    C_DECLARE_PRIVATE(Global, d_ptr);
};

class QCM_INTERFACE_API GlobalWrapper : public QObject {
    Q_OBJECT
    Q_CLASSINFO("DefaultProperty", "datas")
    QML_ELEMENT

    Q_PROPERTY(QQmlListProperty<QObject> datas READ datas FINAL)
    Q_PROPERTY(model::AppInfo info READ info CONSTANT FINAL)
    Q_PROPERTY(QQmlComponent* copy_action_comp READ copy_action_comp WRITE set_copy_action_comp
                   NOTIFY copyActionCompChanged FINAL)
    Q_PROPERTY(QString uuid READ uuid NOTIFY uuidChanged FINAL)

public:
    GlobalWrapper();
    ~GlobalWrapper();

    auto                 datas() -> QQmlListProperty<QObject>;
    auto                 info() -> const model::AppInfo&;
    auto                 copy_action_comp() const -> QQmlComponent*;
    auto                 uuid() const -> QString;
    Q_INVOKABLE QVariant server_url(const model::ItemId&);

Q_SIGNALS:
    void errorOccurred(QString error, StopSignal stop = {});
    void toast(QString text, qint32 duration = 4000, enums::ToastFlags flag = {},
               QObject* action = nullptr, StopSignal stop = {});
    void copyActionCompChanged(StopSignal stop = {});
    void uuidChanged(StopSignal stop = {});
    void playbackLog(enums::PlaybackState state, model::ItemId item, model::ItemId souce,
                     QVariantMap extra = {}, StopSignal stop = {});

public Q_SLOTS:
    void set_copy_action_comp(QQmlComponent*);

private:
    template<typename R, typename... ARGS>
    void connect_from_global(Global* g, R (Global::*g_func)(ARGS...),
                             R (GlobalWrapper::*gw_func)(ARGS...));

    template<typename R, typename... ARGS>
    void connect_to_global(Global* g, R (Global::*g_func)(ARGS...),
                           R (GlobalWrapper::*gw_func)(ARGS...));

    Global*         m_g;
    QList<QObject*> m_datas;
};

} // namespace qcm