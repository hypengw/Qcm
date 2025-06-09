#include "Qcm/query/sync_query.hpp"
#include "Qcm/backend.hpp"
#include "Qcm/app.hpp"
#include "Qcm/util/async.inl"

namespace qcm
{
SyncQuery::SyncQuery(QObject* parent): Query(parent) {}
void SyncQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::SyncReq {};
    req.setProviderId(m_provider_id.id());
    auto self    = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::SyncRsp& el) {
        });
        co_return;
    });
}
auto SyncQuery::providerId() const -> model::ItemId { return m_provider_id; }
void SyncQuery::setProviderId(const model::ItemId& v) {
    m_provider_id = v;
    providerIdChanged();
}

} // namespace qcm

#include <Qcm/query/moc_sync_query.cpp>