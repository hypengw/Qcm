#pragma once

#include <QObject>
#include <QQmlParserStatus>
#include <QQmlEngine>
#include <QAbstractItemModel>

#include <asio/cancellation_signal.hpp>
#include <asio/bind_cancellation_slot.hpp>
#include <asio/use_awaitable.hpp>

#include "asio_helper/watch_dog.h"
#include "ncm/api.h"
#include "ncm/client.h"

#include "Qcm/type.h"

namespace qcm
{

namespace detail
{

ncm::Client get_client();

template<typename M, typename A>
concept modelable =
    requires(M t, typename A::out_type out, typename A::in_type in) { t.handle_output(out, in); };
} // namespace detail

template<typename M, typename A>
concept modelable = detail::modelable<M, A> &&
                    (! std::derived_from<M, QAbstractItemModel> ||
                     requires(M t, qint32 offset) { t.fetchMoreReq(offset); });

class ApiQuerierBase : public QObject, public QQmlParserStatus {
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT
public:
    ApiQuerierBase(QObject* parent = nullptr);
    virtual ~ApiQuerierBase();
    enum Status
    {
        Uninitialized,
        Querying,
        Finished,
        Error
    };
    Q_ENUM(Status);

public:
    Q_PROPERTY(Status status READ status WRITE set_status NOTIFY statusChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool autoReload READ auto_reload WRITE set_auto_reload NOTIFY autoReloadChanged)
    Q_PROPERTY(QObject* data READ data NOTIFY dataChanged)

    Q_INVOKABLE void query() { reload(); }

    Status status() const;
    void   set_status(Status);

    QString error() const;

    bool auto_reload() const;
    void set_auto_reload(bool);

    bool dirty() const;
    bool is_qml_parsing() const;
    void classBegin() override;
    void componentComplete() override;

    virtual QObject* data() const = 0;
    virtual void     reload()     = 0;
    // virtual bool can_relaod() const = 0;

public slots:
    void reload_if_needed() {
        if (! is_qml_parsing() && auto_reload() && dirty()) {
            reload();
            mark_dirty(false);
        }
    }

    void         mark_dirty(bool = true);
    virtual void fetch_more(qint32) {}

signals:
    void statusChanged();
    void errorChanged();
    void autoReloadChanged();
    void dataChanged();

protected:
    void set_error(QString);

    template<typename Ex, typename Fn>
    void spawn(Ex&& ex, Fn&& f) {
        QPointer<ApiQuerierBase> self { this };
        asio::any_io_executor    main_ex { m_main_ex };
        asio::co_spawn(
            ex, m_wdog.watch(ex, std::forward<Fn>(f)), [self, main_ex](std::exception_ptr p) {
                if (! p) return;
                try {
                    std::rethrow_exception(p);
                } catch (const std::exception& e) {
                    std::string e_str = e.what();
                    asio::post(main_ex, [self, e_str]() {
                        if (self) {
                            self->set_error(To<QString>::from(e_str));
                            self->set_status(Status::Error);
                        }
                    });
                    ERROR_LOG("{}", e_str);
                }
            });
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
};

template<typename TApi, typename TModel>
    requires ncm::api::ApiCP<TApi> && modelable<TModel, TApi>
class ApiQuerier : public ApiQuerierBase {
public:
    using api_type   = TApi;
    using out_type   = typename TApi::out_type;
    using in_type    = typename TApi::in_type;
    using model_type = TModel;

    ApiQuerier(QObject* parent)
        : ApiQuerierBase(parent),
          m_model(new model_type(this)),
          m_client(detail::get_client()) {
        if constexpr (std::derived_from<TModel, QAbstractItemModel>) {
            connect(m_model,
                    &TModel::fetchMoreReq,
                    this,
                    &ApiQuerier::fetch_more,
                    Qt::DirectConnection);
        }
    }

    QObject* data() const override { return m_model; }

    void reload() override {
        this->set_status(Status::Querying);
        this->spawn(m_client.get_executor(),
                    [cnt = gen_context()]() mutable -> asio::awaitable<void> {
                        auto& self = cnt.self;
                        auto& cli  = cnt.client;

                        ncm::Result<out_type> out = co_await cli.perform(cnt.api);

                        if (! out) {
                            ERROR_LOG("{}", out.error());
                        }

                        // switch to qt thread
                        co_await asio::post(asio::bind_executor(cnt.main_ex, asio::use_awaitable));
                        if (self) {
                            if (out) {
                                self->model()->handle_output(std::move(out).value(), cnt.api.input);
                                self->set_status(Status::Finished);
                            } else {
                                self->set_error(To<QString>::from(out.error().what()));
                                self->set_status(Status::Error);
                            }
                        }
                        co_return;
                    });
    }

protected:
    api_type&       api() { return m_api; }
    const api_type& api() const { return m_api; }
    model_type*     model() { return m_model; }

private:
    struct Context {
        asio::any_io_executor              main_ex;
        ncm::Client                        client;
        api_type                           api;
        QPointer<ApiQuerier<TApi, TModel>> self;
    };

    Context gen_context() {
        return Context {
            .main_ex = get_executor(),
            .client  = m_client,
            .api     = this->api(),
            .self    = this,
        };
    }

    api_type    m_api;
    model_type* m_model;
    ncm::Client m_client;
};
} // namespace qcm
