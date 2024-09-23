#pragma once

#include <QObject>

#include "core/core.h"
#include "core/expected_helper.h"
#include "asio_qt/qt_executor.h"
#include "qcm_interface/enum.h"

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
    Q_PROPERTY(QObject* data READ data NOTIFY dataChanged FINAL)
    Q_PROPERTY(
        bool forwardError READ forwardError WRITE set_forwardError NOTIFY forwardErrorChanged FINAL)
public:
    QAsyncResult(QObject* parent = nullptr);
    virtual ~QAsyncResult();

    using Status = enums::ApiStatus;
    virtual auto data() const -> QObject*;

    auto status() const -> Status;
    auto error() const -> const QString&;
    bool forwardError() const;
    auto get_executor() -> QtExecutor&;

    template<typename Ex, typename Fn>
    void spawn(Ex&& ex, Fn&& f);

    template<typename T, typename TE>
    void from(const nstd::expected<T, TE>& exp) {
        if (exp) {
            if constexpr (std::is_base_of_v<QObject, std::decay_t<std::remove_pointer_t<T>>> && std::is_pointer_v<T>) {
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

public Q_SLOTS:
    void cancel();
    void set_data(QObject*);
    void set_status(Status);
    void set_error(QString);
    void set_forwardError(bool);
    void hold(QStringView, QObject*);

Q_SIGNALS:
    void dataChanged();
    void statusChanged(Status);
    void errorChanged();
    void forwardErrorChanged();

private:
    auto watch_dog() -> helper::WatchDog&;

    class Private;
    C_DECLARE_PRIVATE(QAsyncResult, d_ptr);
};
} // namespace qcm