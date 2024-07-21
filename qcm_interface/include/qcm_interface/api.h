#pragma once

#include <QObject>
#include <QQmlParserStatus>
#include <QQmlEngine>
#include <QAbstractItemModel>

#include <asio/cancellation_signal.hpp>
#include <asio/bind_cancellation_slot.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/recycling_allocator.hpp>
#include <asio/bind_allocator.hpp>

#include "asio_helper/watch_dog.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/type.h"

namespace qcm
{

namespace detail
{

template<typename M, typename A>
concept modelable =
    requires(M t, typename A::out_type out, typename A::in_type in) { t.handle_output(out, in); };
} // namespace detail

template<typename M, typename A>
concept modelable =
    detail::modelable<M, A> && (! std::derived_from<M, QAbstractItemModel> ||
                                requires(M t, qint32 offset) { t.fetchMoreReq(offset); });

class ApiQuerierBase : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
public:
    ApiQuerierBase(QObject* parent = nullptr);
    virtual ~ApiQuerierBase();
    using Status = enums::ApiStatus;

public:
    Q_PROPERTY(Status status READ status WRITE set_status NOTIFY statusChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool autoReload READ autoReload WRITE set_autoReload NOTIFY autoReloadChanged)
    Q_PROPERTY(QObject* data READ data NOTIFY dataChanged)
    Q_PROPERTY(
        bool forwardError READ forwardError WRITE set_forwardError NOTIFY forwardErrorChanged)

    Q_INVOKABLE void query() { reload(); }

    Status status() const;
    void   set_status(Status);

    QString error() const;

    bool autoReload() const;
    bool forwardError() const;

    bool dirty() const;
    bool is_qml_parsing() const;
    void classBegin() override;
    void componentComplete() override;

    virtual QObject* data() const = 0;
    virtual void     reload()     = 0;
    // virtual bool can_relaod() const = 0;

public slots:
    void set_autoReload(bool);
    void set_forwardError(bool);
    void reload_if_needed() {
        if (! is_qml_parsing() && autoReload() && dirty()) {
            reload();
            mark_dirty(false);
        }
    }

    void         mark_dirty(bool = true);
    virtual void fetch_more(qint32) {}

signals:
    void statusChanged();
    void errorChanged();
    void forwardErrorChanged();
    void autoReloadChanged();
    void dataChanged();

protected:
    void set_error(QString);

    template<typename Ex, typename Fn>
    void spawn(Ex&& ex, Fn&& f) {
        QPointer<ApiQuerierBase> self { this };
        asio::any_io_executor    main_ex { m_main_ex };
        auto                     alloc = asio::recycling_allocator<void>();
        asio::co_spawn(ex,
                       m_wdog.watch(ex, std::forward<Fn>(f), alloc),
                       asio::bind_allocator(alloc, [self, main_ex](std::exception_ptr p) {
                           if (! p) return;
                           try {
                               std::rethrow_exception(p);
                           } catch (const std::exception& e) {
                               std::string e_str = e.what();
                               asio::post(main_ex, [self, e_str]() {
                                   if (self) {
                                       self->set_error(convert_from<QString>(e_str));
                                       self->set_status(Status::Error);
                                   }
                               });
                               ERROR_LOG("{}", e_str);
                           }
                       }));
    }

    void cancel() { m_wdog.cancel(); }

    asio::any_io_executor& get_executor() { return m_main_ex; }

private:
    asio::any_io_executor m_main_ex;
    helper::WatchDog      m_wdog;
    Status                m_status;
    QString               m_error;
    bool                  m_auto_reload;
    bool                  m_qml_parsing;
    bool                  m_dirty;
    bool                  m_forward_error;
};

}