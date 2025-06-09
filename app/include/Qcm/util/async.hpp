#pragma once

#include <QObject>
#include <QObjectBindableProperty>

#include "core/core.h"
#include "core/qasio/qt_executor.h"
#include "Qcm/qml/enum.hpp"
#include "Qcm/util/ex.hpp"
#include "core/qasio/qt_watcher.h"
#include "core/asio/task.h"
#include "core/qstr_helper.h"

namespace helper
{
class WatchDog;
}

namespace qcm
{

class QAsyncResult : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString error READ error NOTIFY errorChanged BINDABLE bindableError FINAL)
    Q_PROPERTY(qcm::enums::ApiStatus status READ status WRITE setStatus NOTIFY statusChanged
                   BINDABLE bindableStatus FINAL)
    Q_PROPERTY(bool querying READ querying NOTIFY queryingChanged BINDABLE bindableQuerying FINAL)
    Q_PROPERTY(QVariant data READ data NOTIFY dataChanged)
    Q_PROPERTY(
        bool forwardError READ forwardError WRITE setForwardError NOTIFY forwardErrorChanged FINAL)
public:
    QAsyncResult(QObject* parent = nullptr);
    virtual ~QAsyncResult();

    using Status = enums::ApiStatus;
    auto data() const -> const QVariant&;
    auto data() -> QVariant&;

    auto qexecutor() const -> QtExecutor&;
    auto pool_executor() const -> asio::thread_pool::executor_type;
    auto status() const -> Status;
    auto bindableStatus() -> QBindable<Status>;
    auto querying() const -> bool;
    auto bindableQuerying() -> QBindable<bool>;
    auto error() const -> const QString&;
    auto bindableError() -> QBindable<QString>;
    bool forwardError() const;
    auto get_executor() -> QtExecutor&;
    auto use_queue() const -> bool;
    void set_use_queue(bool);

    Q_INVOKABLE virtual void reload();
    void                     set_reload_callback(const std::function<void()>&);

    template<typename Fn>
    void spawn(Fn&& f, const std::source_location loc = std::source_location::current());

    template<typename T, typename TE>
    void from(const Result<T, TE>& exp) {
        if (exp) {
            if constexpr (std::is_base_of_v<QObject, std::decay_t<std::remove_pointer_t<T>>> &&
                          std::is_pointer_v<T>) {
                set_data(exp.value());
            } else {
                set_data(QVariant::fromValue(nullptr));
            }
            setStatus(Status::Finished);
        } else {
            setError(convert_from<QString>(exp.error().what()));
            setStatus(Status::Error);
        }
    }

    Q_SLOT void cancel();
    Q_SLOT void setStatus(Status);
    Q_SLOT void setError(const QString&);
    Q_SLOT void setForwardError(bool);
    Q_SLOT void set_data(const QVariant&);
    Q_SLOT void hold(QStringView, QObject*);

    Q_SIGNAL void dataChanged();
    Q_SIGNAL void statusChanged(Status);
    Q_SIGNAL void queryingChanged(bool);
    Q_SIGNAL void errorChanged(QString);
    Q_SIGNAL void forwardErrorChanged();
    Q_SIGNAL void finished();
    Q_SIGNAL void errorOccurred(QString);

    template<typename T, typename Err>
    void check(const Result<T, Err>& res) {
        if (! res) {
            setError(QString::fromStdString(res.error().what()));
            setStatus(Status::Error);
        }
    }

private:

    void  push(std::function<task<void>()>, const std::source_location& loc);
    usize size() const;

    auto watch_dog() -> helper::WatchDog&;

    class Private;
    C_DECLARE_PRIVATE(QAsyncResult, d_ptr);
};

template<typename T, typename Self>
class QAsyncResultExtra {
public:
    using value_type = std::conditional_t<std::is_base_of_v<QObject, T>, std::add_pointer_t<T>, T>;
    using const_reference_value_type =
        std::conditional_t<std::is_base_of_v<QObject, T>, std::add_pointer_t<T>,
                           std::add_lvalue_reference_t<std::add_const_t<T>>>;
    auto tdata() const {
        return static_cast<const QAsyncResult*>(static_cast<const Self*>(this))
            ->data()
            .template value<value_type>();
    }
    auto tdata() {
        return static_cast<QAsyncResult*>(static_cast<Self*>(this))
            ->data()
            .template value<value_type>();
    }
    void set_tdata(const_reference_value_type val) {
        auto self = static_cast<Self*>(this);
        if constexpr (std::is_base_of_v<QObject, T>) {
            auto old = tdata();
            self->set_data(QVariant::fromValue(val));
            if (old) {
                old->deleteLater();
            }
        } else {
            self->set_data(QVariant::fromValue(val));
        }
    }

    template<typename U, typename E = typename std::remove_reference_t<U>::error_type>
        requires(std::convertible_to<U, Result<T, E>> || std::convertible_to<U, Result<T&, E>>)
    void set(U&& res) {
        auto self = static_cast<Self*>(this);
        if (res) {
            set_tdata(*res);
            self->setStatus(QAsyncResult::Status::Finished);
        } else {
            self->setError(rstd::into(std::format("{}", res.unwrap_err_unchecked())));
            self->setStatus(QAsyncResult::Status::Error);
        }
    }

    template<typename U, typename F>
    void inspect_set(U&& res, F&& f) {
        auto self = static_cast<Self*>(this);
        if (res) {
            res.inspect(std::forward<F>(f));
            self->setStatus(QAsyncResult::Status::Finished);
        } else {
            self->setError(rstd::into(std::format("{}", res.unwrap_err_unchecked())));
            self->setStatus(QAsyncResult::Status::Error);
        }
    }
};
} // namespace qcm