#include "Qcm/query/provider_query.hpp"

#include "qcm_interface/async.inl"

#include "Qcm/app.h"

namespace qcm::query
{

AddProviderQuery::AddProviderQuery(QObject* parent): QAsyncResultT<msg::Rsp>(parent) {
    set_forwardError(true);
}
void AddProviderQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    spawn([self, backend, req = m_req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->set(std::move(rsp));
        co_return;
    });
}
auto AddProviderQuery::req() -> msg::AddProviderReq& { return m_req; }
void AddProviderQuery::setReq(msg::AddProviderReq& req) {
    if (ycore::cmp_exchange(m_req, req)) {
        reqChanged();
    }
}

ProviderMetasQuery::ProviderMetasQuery(QObject* parent)
    : QAsyncResultT<msg::GetProviderMetasRsp>(parent) {
    set_forwardError(true);
}
void ProviderMetasQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    spawn([self, backend] -> task<void> {
        auto rsp = co_await backend->send(msg::GetProviderMetasReq {});
        co_await qcm::qexecutor_switch();
        self->set(std::move(rsp));
        co_return;
    });
}

} // namespace qcm::query

#include <Qcm/query/moc_provider_query.cpp>
