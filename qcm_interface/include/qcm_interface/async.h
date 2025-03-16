#pragma once

#include <QObject>
#include <QObjectBindableProperty>

#include "core/core.h"
#include "core/expected_helper.h"
#include "asio_qt/qt_executor.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/ex.h"
#include "asio_qt/qt_watcher.h"
#include "asio_helper/task.h"

namespace helper
{
class WatchDog;
}

namespace qcm
{

class QCM_INTERFACE_API QAsyncResult : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString error READ error NOTIFY errorChanged BINDABLE bindableError FINAL)
    Q_PROPERTY(qcm::enums::ApiStatus status READ status WRITE set_status NOTIFY statusChanged
                   BINDABLE bindableStatus FINAL)
    Q_PROPERTY(QVariant data READ data NOTIFY dataChanged)
    Q_PROPERTY(
        bool forwardError READ forwardError WRITE set_forwardError NOTIFY forwardErrorChanged FINAL)
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
    void from(const nstd::expected<T, TE>& exp) {
        if (exp) {
            if constexpr (std::is_base_of_v<QObject, std::decay_t<std::remove_pointer_t<T>>> &&
                          std::is_pointer_v<T>) {
                set_data(exp.value());
            } else {
                set_data(QVariant::fromValue(nullptr));
            }
            set_status(Status::Finished);
        } else {
            set_error(convert_from<QString>(exp.error().what()));
            set_status(Status::Error);
        }
    }

    Q_SLOT virtual void set_data(const QVariant&);

    Q_SLOT void cancel();
    Q_SLOT void set_status(Status);
    Q_SLOT void set_error(const QString&);
    Q_SLOT void set_forwardError(bool);
    Q_SLOT void hold(QStringView, QObject*);

    Q_SIGNAL void dataChanged();
    Q_SIGNAL void statusChanged(Status);
    Q_SIGNAL void errorChanged(QString);
    Q_SIGNAL void forwardErrorChanged();
    Q_SIGNAL void finished();
    Q_SIGNAL void errorOccurred(QString);

    template<typename T, typename Err>
    void check(const nstd::expected<T, Err>& res) {
        if (! res) {
            set_error(QString::fromStdString(res.error().what()));
            set_status(Status::Error);
        }
    }

private:
    void  push(std::function<task<void>()>, const std::source_location& loc);
    usize size() const;

    auto watch_dog() -> helper::WatchDog&;

    class Private;
    C_DECLARE_PRIVATE(QAsyncResult, d_ptr);
};

template<typename T, typename Base = QAsyncResult>
    requires std::is_base_of_v<QAsyncResult, Base>
class QAsyncResultT : public Base {
public:
    using value_type = std::conditional_t<std::is_base_of_v<QObject, T>, std::add_pointer_t<T>, T>;
    using const_reference_value_type =
        std::conditional_t<std::is_base_of_v<QObject, T>, std::add_pointer_t<T>,
                           std::add_lvalue_reference_t<std::add_const_t<T>>>;

    QAsyncResultT(ycore::monostate, QObject* parent, const_reference_value_type t): Base(parent) {
        set_tdata(t);
    }

    template<typename... Arg>
        requires(std::is_base_of_v<QObject, T> && std::is_constructible_v<T, Arg..., QObject*>) ||
                (! std::is_base_of_v<QObject, T> && std::is_constructible_v<T, Arg...>)
    QAsyncResultT(QObject* parent, Arg&&... arg): Base(parent) {
        if constexpr (std::is_base_of_v<QObject, T>) {
            set_tdata(new T(std::forward<Arg>(arg)..., this));
        } else {
            set_tdata(T(std::forward<Arg>(arg)...));
        }
    }

    auto tdata() const { return this->data().template value<value_type>(); }
    auto tdata() { return this->data().template value<value_type>(); }
    void set_tdata(const_reference_value_type val) {
        if constexpr (std::is_base_of_v<QObject, T>) {
            auto old = tdata();
            QAsyncResult::set_data(QVariant::fromValue(val));
            if (old) {
                old->deleteLater();
            }
        } else {
            QAsyncResult::set_data(QVariant::fromValue(val));
        }
    }

    template<typename U, typename E = typename std::remove_reference_t<U>::error_type>
        requires(std::convertible_to<U, Result<T, E>> || std::convertible_to<U, Result<T&, E>>)
    void set(U&& res) {
        if (res) {
            set_tdata(*res);
        } else {
            this->set_error(convert_from<QString>(std::format("{}", res.unwrap_err_unchecked())));
        }
    }

private:
    void set_data(const QVariant&) override { _assert_rel_(false); }
};
} // namespace qcm