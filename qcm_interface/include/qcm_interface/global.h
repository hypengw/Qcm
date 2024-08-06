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

class QCM_INTERFACE_API Global : public QObject {
    Q_OBJECT
    Q_CLASSINFO("DefaultProperty", "datas")
    QML_NAMED_ELEMENT(GlobalImpl)

    Q_PROPERTY(QQmlListProperty<QObject> datas READ datas FINAL)
    Q_PROPERTY(model::AppInfo info READ info CONSTANT FINAL)
public:
    using pool_executor_t = asio::thread_pool::executor_type;
    using qt_executor_t   = QtExecutor;

    static auto instance() -> Global*;
    static void setInstance(Global*);

    Global();
    ~Global();

    auto client(std::string_view name, std::optional<std::function<std::any()>> = std::nullopt)
        -> std::any;
    auto qexecutor() -> qt_executor_t&;
    auto pool_executor() -> pool_executor_t;
    auto session() -> rc<request::Session>;

    auto info() const -> const model::AppInfo&;

    void join();

    auto datas() -> QQmlListProperty<QObject>;

Q_SIGNALS:
    void errorOccurred(QString);

private:
    class Private;
    C_DECLARE_PRIVATE(Global, d_ptr);
};

} // namespace qcm