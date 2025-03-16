#include "Qcm/query/provider_meta.h"

#include "qcm_interface/async.inl"

#include "Qcm/app.h"

namespace qcm::query
{

ProviderMetasQuery::ProviderMetasQuery(QObject* parent)
    : QAsyncResultT<msg::GetProviderMetasRsp>(parent) {}
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

#include <Qcm/query/moc_provider_meta.cpp>
