#include "Qcm/query/mix_query.hpp"

#include "Qcm/backend.hpp"
#include "Qcm/app.hpp"
#include "Qcm/store.hpp"

#include "Qcm/util/async.inl"

namespace qcm
{


MixesQuery::MixesQuery(QObject* parent): QueryList<model::MixListModel>(parent) {
    this->connectSyncFinished();
}

void MixesQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetMixsReq {};
    req.setPage(0);
    req.setPageSize((offset() + 1) * limit());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::GetMixsRsp& el) {
            auto t = self->tdata();
            t->setHasMore(false);
            t->resetModel(el.items());
            t->setHasMore(el.hasMore());
        });
        co_return;
    });
}

void MixesQuery::fetchMore(qint32) {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetMixsReq {};
    req.setPage(offset() + 1);
    req.setPageSize(limit());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto offset = req.page();
        auto rsp    = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self, offset](msg::GetMixsRsp& el) {
            auto view = std::views::transform(el.items(), [](auto&& el) {
                return model::Mix(el);
            });
            self->tdata()->extend(view);
            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

MixQuery::MixQuery(QObject* parent): Query<model::MixStoreItem>(parent) {}

auto MixQuery::itemId() const -> model::ItemId { return m_item_id; }

void MixQuery::setItemId(model::ItemId in) {
    if (ycore::cmp_exchange(m_item_id, in)) {
        itemIdChanged();
    }
}

void MixQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetMixReq {};
    req.setId_proto(m_item_id.id());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::GetMixRsp& el) {
            auto store = AppStore::instance();
            self->tdata()->setItem(el.item());
            merge_store_extra(store->mixes, el.item().id_proto(), el.extra());
        });
        co_return;
    });
}

} // namespace qcm

#include <Qcm/query/moc_mix_query.cpp>
