module;
#include "Qcm/query/sync_query.moc.h"
module qcm;
import :query.sync;

namespace qcm
{

SyncQuery::SyncQuery(QObject* parent): Query(parent) {}
void SyncQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::SyncReq {};
    req.setProviderId(m_provider_id.id());
    auto self = QWatcher { this };
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
    if (ycore::cmp_set(m_provider_id, v)) {
        m_provider_id = v;
        providerIdChanged();
    }
}

SyncItemQuery::SyncItemQuery(QObject* parent): Query(parent) {}
void SyncItemQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::SyncItemReq {};
    req.setId_proto(m_item_id.id());

    auto self = QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::Rsp&) {
        });
        co_return;
    });
}
auto SyncItemQuery::itemId() const -> model::ItemId { return m_item_id; }
void SyncItemQuery::setItemId(const model::ItemId& v) {
    if (ycore::cmp_set(m_item_id, v)) {
        m_item_id = v;
        itemIdChanged();
    }
}

} // namespace qcm

#include "Qcm/query/sync_query.moc.cpp"
