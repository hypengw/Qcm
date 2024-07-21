#pragma once

#include <QObject>
#include <any>

#include <asio/thread_pool.hpp>
#include <asio/steady_timer.hpp>

#include "core/core.h"
#include "asio_qt/qt_executor.h"

namespace request
{
class Session;
}
namespace qcm
{

class Global : public QObject {
    Q_OBJECT

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

    void join();

Q_SIGNALS:
    void errorOccurred(QString);

private:
    class Private;
    up<Private> d_ptr;
    C_DECLARE_PRIVATE(Global, d_ptr);
};

} // namespace qcm