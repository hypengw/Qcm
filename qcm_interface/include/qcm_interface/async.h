#pragma once

#include <QObject>

#include "core/core.h"
#include "core/expected_helper.h"
#include "asio_qt/qt_executor.h"
#include "qcm_interface/enum.h"
#include "asio_helper/task.h"
#include <asio/thread_pool.hpp>

namespace helper
{
class WatchDog;
}

namespace qcm
{

class QCM_INTERFACE_API QAsyncResult : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString error READ error NOTIFY errorChanged FINAL)
    Q_PROPERTY(Status status READ status WRITE set_status NOTIFY statusChanged FINAL)
    Q_PROPERTY(QVariant data READ data NOTIFY dataChanged)
    Q_PROPERTY(
        bool forwardError READ forwardError WRITE set_forwardError NOTIFY forwardErrorChanged FINAL)
public:
    QAsyncResult(QObject* parent = nullptr);
    virtual ~QAsyncResult();

    using Status = enums::ApiStatus;
    virtual auto data() const -> QVariant;

    auto qexecutor() const -> QtExecutor&;
    auto pool_executor() const -> asio::thread_pool::executor_type;
    auto status() const -> Status;
    auto error() const -> const QString&;
    bool forwardError() const;
    auto get_executor() -> QtExecutor&;
    auto use_queue() const -> bool;
    void set_use_queue(bool);

    Q_INVOKABLE virtual void reload();
    void                     set_reload_callback(const std::function<void()>&);

    template<typename Ex, typename Fn>
    void spawn(Ex&& ex, Fn&& f, const std::source_location loc = {});

    template<typename T, typename TE>
    void from(const nstd::expected<T, TE>& exp) {
        if (exp) {
            if constexpr (std::is_base_of_v<QObject, std::decay_t<std::remove_pointer_t<T>>> &&
                          std::is_pointer_v<T>) {
                set_data(exp.value());
            } else {
                set_data(nullptr);
            }
            set_status(Status::Finished);
        } else {
            set_error(convert_from<QString>(exp.error().what()));
            set_status(Status::Error);
        }
    }

    Q_SLOT void cancel();
    Q_SLOT void set_data(QObject*);
    Q_SLOT void set_status(Status);
    Q_SLOT void set_error(QString);
    Q_SLOT void set_forwardError(bool);
    Q_SLOT void hold(QStringView, QObject*);

    Q_SIGNAL void dataChanged();
    Q_SIGNAL void statusChanged(Status);
    Q_SIGNAL void errorChanged();
    Q_SIGNAL void forwardErrorChanged();
    Q_SIGNAL void finished();
    Q_SIGNAL void errorOccurred(QString);

private:
    void  push(std::function<task<void>()>, const std::source_location& loc);
    usize size() const;

    auto watch_dog() -> helper::WatchDog&;

    class Private;
    C_DECLARE_PRIVATE(QAsyncResult, d_ptr);
};
} // namespace qcm