#pragma once

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

#include "qcm_interface/sql/cache_sql.h"
#include "qcm_interface/sql/collection_sql.h"

#include "qcm_interface/metadata.h"
#include "qcm_interface/client.h"
#include "qcm_interface/model/user_model.h"
#include "qcm_interface/model/session.h"
#include "qcm_interface/model/busy_info.h"
#include "qcm_interface/state/app_state.h"

namespace request
{
class Session;
}
namespace qcm
{
class App;
class PluginModel;
class QcmPluginInterface;

namespace db
{
class ItemSqlBase;
}

struct StopSignal {
    bool val { false };
};

QCM_INTERFACE_API auto qml_dyn_count() -> std::atomic<i32>&;
QCM_INTERFACE_API auto create_item(QQmlEngine* engine, const QJSValue& url_or_comp,
                                   const QVariantMap& props, QObject* parent) -> QObject*;

QCM_INTERFACE_API auto image_provider_url(const QUrl& url, const QString& provider) -> QUrl;
QCM_INTERFACE_API auto parse_image_provider_url(const QUrl& url) -> std::tuple<QUrl, QString>;

class GlobalWrapper;
class QCM_INTERFACE_API Global : public QObject {
    Q_OBJECT
    Q_PROPERTY(qcm::model::AppInfo info READ info CONSTANT FINAL)
    Q_PROPERTY(QQmlComponent* copy_action_comp READ copy_action_comp WRITE set_copy_action_comp
                   NOTIFY copyActionCompChanged FINAL)

    Q_PROPERTY(qcm::UserModel* userModel READ user_model CONSTANT FINAL)
    Q_PROPERTY(qcm::model::Session* session READ qsession NOTIFY sessionChanged FINAL)
    Q_PROPERTY(qcm::model::BusyInfo* busy READ busy_info CONSTANT FINAL)
    Q_PROPERTY(qcm::state::AppState* appState READ app_state CONSTANT FINAL)

    friend class GlobalWrapper;
    friend class App;
    friend class PluginModel;

public:
    using pool_executor_t = asio::thread_pool::executor_type;
    using qt_executor_t   = QtExecutor;
    using Metadata        = player::Metadata;
    using Client          = qcm::Client;

    static auto instance() -> Global*;

    Global();
    ~Global();

    auto qexecutor() -> qt_executor_t&;
    auto pool_executor() -> pool_executor_t;
    auto session() -> rc<request::Session>;
    auto qsession() const -> model::Session*;
    auto busy_info() const -> model::BusyInfo*;
    auto app_state() const -> state::AppState*;

    auto get_cache_sql() const -> rc<media_cache::DataBase>;
    auto get_collection_sql() const -> rc<db::ColletionSqlBase>;
    auto get_item_sql() const -> rc<db::ItemSqlBase>;

    auto user_model() const -> UserModel*;

    auto copy_action_comp() const -> QQmlComponent*;
    auto uuid() const -> const QUuid&;

    auto info() const -> const model::AppInfo&;

    auto get_metadata(const std::filesystem::path&) const -> Metadata;

    auto user_agent() const -> std::string_view;
    auto plugin(QStringView) const -> std::optional<std::reference_wrapper<QcmPluginInterface>>;

    void join();

    Q_INVOKABLE QVariant server_url(const model::ItemId&);

Q_SIGNALS:
    void errorOccurred(QString error, StopSignal stop = {});
    void copyActionCompChanged(StopSignal stop = {});
    void uuidChanged(StopSignal stop = {});
    void sessionChanged(StopSignal stop = {});

public Q_SLOTS:
    void set_copy_action_comp(QQmlComponent*);
    void switch_user(model::UserAccount*);
    void load_user();
    void save_user();

private:
    using MetadataImpl = std::function<Metadata(const std::filesystem::path&)>;
    void set_uuid(const QUuid&);
    void set_session(model::Session*);

    void set_cache_sql(rc<media_cache::DataBase>);
    void set_collection_sql(rc<db::ColletionSqlBase>);
    void set_album_sql(rc<db::ItemSqlBase>);

    void set_metadata_impl(const MetadataImpl&);
    auto load_plugin(const std::filesystem::path&) -> bool;

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
    Q_PROPERTY(UserModel* userModel READ user_model CONSTANT FINAL)
    Q_PROPERTY(model::Session* session READ qsession NOTIFY sessionChanged FINAL)
    Q_PROPERTY(model::BusyInfo* busy READ busy_info CONSTANT FINAL)
    Q_PROPERTY(state::AppState* appState READ app_state CONSTANT FINAL)

public:
    GlobalWrapper();
    ~GlobalWrapper();

    auto datas() -> QQmlListProperty<QObject>;
    auto info() -> const model::AppInfo&;
    auto user_model() const -> UserModel*;
    auto copy_action_comp() const -> QQmlComponent*;
    auto uuid() const -> QString;
    auto qsession() const -> model::Session*;
    auto busy_info() const -> model::BusyInfo*;
    auto app_state() const -> state::AppState*;

    Q_INVOKABLE QVariant server_url(const model::ItemId&);

Q_SIGNALS:
    void errorOccurred(QString error, StopSignal stop = {});
    void copyActionCompChanged(StopSignal stop = {});
    void uuidChanged(StopSignal stop = {});
    void sessionChanged(StopSignal stop = {});

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