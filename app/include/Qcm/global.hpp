#pragma once

#include <filesystem>

#include <QtCore/QObject>
#include <QtQml/QQmlListProperty>
#include <QtQml/QQmlComponent>

#include <asio/thread_pool.hpp>
#include <asio/steady_timer.hpp>

#include "core/core.h"
#include "core/qasio/qt_executor.h"
#include "core/core.h"
#include "Qcm/model/app_info.hpp"
#include "Qcm/qml/enum.hpp"

#include "player/metadata.h"
#include "Qcm/status/app_state.hpp"
Q_MOC_INCLUDE("Qcm/player.hpp")

import ncrequest;

namespace qcm
{
class App;
class PluginModel;
class QcmPluginInterface;
class UserModel;
class Player;

namespace db
{
class ItemSqlBase;
}

struct StopSignal {
    bool val { false };
};

auto image_provider_url(const QUrl& url, const QString& provider) -> QUrl;
auto parse_image_provider_url(const QUrl& url) -> std::tuple<QUrl, QString>;

class GlobalWrapper;
class Global : public QObject {
    Q_OBJECT
    friend class GlobalWrapper;
    friend class App;
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

private:
    using MetadataImpl = std::function<Metadata(const std::filesystem::path&)>;
    void set_uuid(const QUuid&);

    void set_metadata_impl(const MetadataImpl&);

    static void setInstance(Global*);

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

} // namespace qcm