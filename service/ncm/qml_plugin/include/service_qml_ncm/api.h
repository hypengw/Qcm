#pragma once

#include "ncm/api.h"
#include "ncm/client.h"

#include "qcm_interface/api.h"
#include "asio_qt/qt_executor.h"
#include "service_qml_ncm/client.h"

namespace qcm
{

namespace detail
{
ncm::Client get_client();
}

template<typename TApi, typename TModel>
    requires ncm::api::ApiCP<TApi> //&& modelable<TModel, TApi>
class ApiQuerier : public ApiQuerierBase {
public:
    using api_type   = TApi;
    using out_type   = typename TApi::out_type;
    using in_type    = typename TApi::in_type;
    using model_type = TModel;

    ApiQuerier(QObject* parent)
        : ApiQuerierBase(parent), m_model(new model_type(this)) {
        if constexpr (std::derived_from<TModel, QAbstractItemModel>) {
            connect(m_model,
                    &TModel::fetchMoreReq,
                    this,
                    &ApiQuerier::fetch_more,
                    Qt::DirectConnection);
        }
    }

    model_type* data() const override { return m_model; }

    void reload() override {
        // co_spawn need strand for cancel
        auto cnt = gen_context();
        if (! cnt) {
            cancel();
            set_error("session not valid");
            set_status(Status::Error);
            return;
        }

        auto ex = asio::make_strand(cnt->client.get_executor());
        this->set_status(Status::Querying);
        this->spawn(ex, [cnt = cnt.value()]() mutable -> asio::awaitable<void> {
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
                        handle_output(*self->model(), out.value());
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
    model_type*     model() { return m_model; }

private:
    struct Context {
        QtExecutor                         main_ex;
        ncm::Client                        client;
        api_type                           api;
        QPointer<ApiQuerier<TApi, TModel>> self;
    };

    auto gen_context() -> std::optional<Context> {
        return session()->client().and_then([this](auto c) -> std::optional<Context> {
            if (c.api->provider != ncm::qml::provider) {
                return std::nullopt;
            }
            return Context {
                .main_ex = get_executor(),
                .client  = ncm::qml::get_ncm_client(c),
                .api     = this->api(),
                .self    = this,
            };
        });
    }

    api_type    m_api;
    model_type* m_model;
};
} // namespace qcm
