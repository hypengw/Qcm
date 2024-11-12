#pragma once

#include "ncm/api.h"
#include "ncm/client.h"

#include "qcm_interface/api.h"
#include "asio_qt/qt_executor.h"
#include "asio_qt/qt_watcher.h"
#include "service_qml_ncm/client.h"

namespace qcm
{

template<typename TApi, typename TModel>
    requires ncm::api::ApiCP<TApi> //&& modelable<TModel, TApi>
class ApiQuerier : public QAsyncResultT<TModel, ApiQuerierBase> {
public:
    using api_type   = TApi;
    using out_type   = typename TApi::out_type;
    using in_type    = typename TApi::in_type;
    using model_type = TModel;
    using Status     = QAsyncResult::Status;

    ApiQuerier(QObject* parent): QAsyncResultT<TModel, ApiQuerierBase>(parent) {
        if constexpr (requires() { TModel::fetchMoreReq; }) {
            connect(this->tdata(),
                    &TModel::fetchMoreReq,
                    this,
                    &ApiQuerier::fetch_more,
                    Qt::DirectConnection);
        }
    }

    virtual void handle_output(const out_type&) {}
    void         reload() override {
        // co_spawn need strand for cancel
        auto cnt = gen_context();
        if (! cnt) {
            this->cancel();
            ERROR_LOG("session not valid");
            this->set_error("session not valid");
            this->set_status(Status::Error);
            return;
        }

        this->set_status(Status::Querying);
        this->spawn([cnt = cnt.value()]() mutable -> asio::awaitable<void> {
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
                    if constexpr (modelable<TModel, TApi>) {
                        self->model()->handle_output(std::move(out).value(), cnt.api.input);
                    } else {
                        self->handle_output(out.value());
                    }
                    self->set_status(Status::Finished);
                } else {
                    self->set_error(convert_from<QString>(out.error().what()));
                    self->set_status(Status::Error);
                }
            }
            co_return;
        });
    }

protected:
    api_type&       api() { return m_api; }
    const api_type& api() const { return m_api; }
    model_type*     model() { return this->tdata(); }
    auto client() { return this->session()->client().and_then(ncm::qml::get_ncm_client); }

private:
    struct Context {
        QtExecutor                                 main_ex;
        ncm::Client                                client;
        api_type                                   api;
        helper::QWatcher<ApiQuerier<TApi, TModel>> self;
    };

    auto gen_context() -> std::optional<Context> {
        return client().transform([this](auto c) {
            return Context {
                .main_ex = this->get_executor(),
                .client  = c,
                .api     = this->api(),
                .self    = this,
            };
        });
    }
    api_type m_api;
};
} // namespace qcm
