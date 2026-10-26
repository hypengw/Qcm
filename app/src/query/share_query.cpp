module;
#include "Qcm/query/share_query.moc.h"
module qcm;
import :query.share;

namespace qcm
{
ShareLinkQuery::ShareLinkQuery(QObject* parent): Query(parent) {
    setForwardError(false);
    connect_requet_reload(&ShareLinkQuery::itemIdChanged, this);
}
model::ItemId ShareLinkQuery::itemId() const { return m_item_id; }
void          ShareLinkQuery::setItemId(model::ItemId id) {
    if (! ycore::cmp_set(m_item_id, id)) return;
    invalidateRequests();
    set_tdata({});
    setStatus(Status::Uninitialized);
    itemIdChanged();
}
void ShareLinkQuery::reload() {
    if (! m_item_id.valid()) return;
    setStatus(Status::Querying);
    auto                 backend = App::instance()->backend();
    msg::GetShareLinkReq request;
    request.setId_proto(m_item_id.id());
    request.setItemType(rstd::into(m_item_id.type()));
    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, request, revision]() mutable -> task<void> {
        auto response = co_await backend->send(std::move(request));
        if (! co_await QAsyncResult::qexecutor() || ! self || ! self->isCurrentRequest(revision))
            co_return;
        self->set(std::move(response));
    });
}
} // namespace qcm
#include "Qcm/query/share_query.moc.cpp"
