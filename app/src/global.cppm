module;
#include "Qcm/macro_qt.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/global.moc"
#endif

#include "player/metadata.h"
#include "core/log.h"

export module qcm:global;
export import :action;
export import :qml.enums;
export import :util.mem;
export import :util.global_static;
export import :player;
export import qextra;
import ncrequest;

using rstd::sync::Arc;


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

    auto get_metadata(const cppstd::filesystem::path&) const -> Metadata;

    void join();

    Q_SIGNAL void errorOccurred(QString error, StopSignal stop = {});
    Q_SIGNAL void copyActionCompChanged(StopSignal stop = {});
    Q_SIGNAL void uuidChanged(StopSignal stop = {});
    Q_SIGNAL void sessionChanged(StopSignal stop = {});

    using MetadataImpl = cppstd::function<Metadata(const cppstd::filesystem::path&)>;
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

    rc<ncrequest::Session> session;

    QUuid uuid;

    MetadataImpl metadata_impl;

    Player* player;

    mutable cppstd::mutex mutex;
};
} // namespace qcm
