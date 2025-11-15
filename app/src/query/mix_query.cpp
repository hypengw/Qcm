#include "Qcm/query/mix_query.hpp"

#include "Qcm/backend.hpp"
#include "Qcm/app.hpp"
#include "Qcm/store.hpp"

#include "Qcm/util/async.inl"

namespace qcm
{

MixesQuery::MixesQuery(QObject* parent): QueryList(parent) {
    auto app = App::instance();
    this->tdata()->set_store(this->tdata(), app->store()->mixes);
    this->connectSyncFinished();
}

void MixesQuery::reload() {
    setStatus(Status::Querying);
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
    setStatus(Status::Querying);
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

MixQuery::MixQuery(QObject* parent): Query(parent) {}

auto MixQuery::itemId() const -> model::ItemId { return m_item_id; }

void MixQuery::setItemId(model::ItemId in) {
    if (ycore::cmp_set(m_item_id, in)) {
        itemIdChanged();
    }
}

void MixQuery::reload() {
    setStatus(Status::Querying);
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

MixSongsQuery::MixSongsQuery(QObject* parent): QueryList(parent) {
    auto app = App::instance();
    this->tdata()->set_store(this->tdata(), app->store()->songs);
    this->connectSyncFinished();
    this->setLimit(1000);
}

auto MixSongsQuery::itemId() const -> model::ItemId { return m_item_id; }

void MixSongsQuery::setItemId(model::ItemId in) {
    if (ycore::cmp_set(m_item_id, in)) {
        itemIdChanged();
    }
}

void MixSongsQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetMixSongsReq {};
    req.setId_proto(m_item_id.id());
    req.setPage(offset() + 1);
    req.setPageSize(limit());
    req.setSort((msg::model::SongSortGadget::SongSort)sort());
    req.setSortAsc(asc());

    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto offset = req.page();
        auto rsp    = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self, offset](msg::GetMixSongsRsp& el) {
            auto view = std::views::transform(el.items(), [](auto&& el) {
                return model::Song(el);
            });

            self->tdata()->extend(view);
            auto store = AppStore::instance();

            for (qsizetype i = 0; i < el.extras().size(); i++) {
                auto id = el.items().at(i).id_proto();
                merge_store_extra(store->songs, id, el.extras().at(i));
            }
            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

auto MixesQuery::filters() const -> const QList<msg::filter::MixFilter>& { return m_filters; }
void MixesQuery::setFilters(const QList<msg::filter::MixFilter>& filters) {
    m_filters = filters;
    filtersChanged();
}

CreateMixQuery::CreateMixQuery(QObject* parent): Query(parent) {
    connect(this, &CreateMixQuery::mixCreated, Notifier::instance(), &Notifier::mixCreated);
}
void CreateMixQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::CreateMixReq {};
    req.setName(m_name);
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto name = req.name();
        auto rsp  = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();

        self->inspect_set(rsp, [self, name](auto&) {
            self->mixCreated(name);
        });

        co_return;
    });
}
auto CreateMixQuery::name() const -> QString { return m_name; }
void CreateMixQuery::setName(const QString& in) {
    if (ycore::cmp_set(m_name, in)) {
        nameChanged();
    }
}

DeleteMixQuery::DeleteMixQuery(QObject* parent): Query(parent) {}
auto DeleteMixQuery::ids() const -> std::vector<model::ItemId> { return m_ids; }
void DeleteMixQuery::setIds(const std::vector<model::ItemId>& ids) {
    m_ids = ids;
    idsChanged();
}
void DeleteMixQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::DeleteMixReq {};
    auto view    = std::views::transform(m_ids, [](const auto& el) {
        return el.id();
    });
    req.setIds({ view.begin(), view.end() });
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();

        self->inspect_set(rsp, [self](auto&) {
            Notifier::instance()->mixDeleted();
        });

        co_return;
    });
}

AddToMixQuery::AddToMixQuery(QObject* parent): Query(parent) {}
void AddToMixQuery::reload() {}

RemoveFromMixQuery::RemoveFromMixQuery(QObject* parent): Query(parent) {}
void RemoveFromMixQuery::reload() {}

} // namespace qcm

#include <Qcm/query/moc_mix_query.cpp>
