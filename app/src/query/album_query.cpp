#include "Qcm/query/album_query.hpp"

#include "Qcm/backend.hpp"
#include "Qcm/app.hpp"
#include "Qcm/store.hpp"

#include "Qcm/util/async.inl"
#include "Qcm/status/provider_status.hpp"

namespace qcm
{

AlbumsQuery::AlbumsQuery(QObject* parent): QueryList(parent) {
    // set_use_queue(true);
    auto app = App::instance();
    this->tdata()->set_store(this->tdata(), app->store()->albums);
    this->connectSyncFinished();
    this->connect_requet_reload(&LibraryStatus::activedIdsChanged, app->libraryStatus());
    this->connect_requet_reload(&AlbumsQuery::filtersChanged, this);
}

auto AlbumsQuery::filters() const -> const QList<msg::filter::AlbumFilter>& { return m_filters; }
void AlbumsQuery::setFilters(const QList<msg::filter::AlbumFilter>& filters) {
    m_filters = filters;
    filtersChanged();
}

void AlbumsQuery::reload() {
    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetAlbumsReq {};
    req.setLibraryId(app->libraryStatus()->activedIds());
    req.setPage(0);
    req.setPageSize((offset() + 1) * limit());
    req.setSort((msg::model::AlbumSortGadget::AlbumSort)sort());
    req.setSortAsc(asc());
    req.setFilters(m_filters);

    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::GetAlbumsRsp& el) {
            auto t = self->tdata();
            t->setHasMore(false);
            auto view = std::views::transform(el.items(), [](auto& el) {
                return model::Album { el };
            });
            t->sync(view);
            auto store = AppStore::instance();
            for (qsizetype i = 0; i < el.extras().size(); i++) {
                auto id = el.items().at(i).id_proto();
                merge_store_extra(store->albums, id, el.extras().at(i));
            }
            t->setHasMore(el.hasMore());
        });
        co_return;
    });
}

void AlbumsQuery::fetchMore(qint32) {
    if (noMore()) return;

    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetAlbumsReq {};
    req.setLibraryId(app->libraryStatus()->activedIds());
    req.setPage(offset() + 1);
    req.setPageSize(limit());
    req.setSort((msg::model::AlbumSortGadget::AlbumSort)sort());
    req.setSortAsc(asc());
    req.setFilters(m_filters);

    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto offset = req.page();
        auto rsp    = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self, offset](msg::GetAlbumsRsp& el) {
            auto view = std::views::transform(el.items(), [](auto&& el) {
                return model::Album(el);
            });

            self->tdata()->extend(view);
            auto store = AppStore::instance();

            for (qsizetype i = 0; i < el.extras().size(); i++) {
                auto id = el.items().at(i).id_proto();
                merge_store_extra(store->albums, id, el.extras().at(i));
            }
            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

AlbumQuery::AlbumQuery(QObject* parent): QueryList(parent) {
    this->tdata()->set_store(this->tdata(), App::instance()->store()->songs);
}
auto AlbumQuery::itemId() const -> model::ItemId { return m_item_id; }
void AlbumQuery::setItemId(model::ItemId in) {
    if (ycore::cmp_exchange(m_item_id, in)) {
        itemIdChanged();
    }
}

void AlbumQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetAlbumReq {};
    req.setId_proto(m_item_id.id());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::GetAlbumRsp& el) {
            auto store = App::instance()->store();
            auto t     = self->tdata();
            t->setAlbum(el.item());

            i64 disc = 1;
            for (auto& el : el.songs()) {
                disc = std::max<i64>(el.discNumber(), disc);
            }
            t->setDiscCount(disc);
            t->resetModel(el.songs());

            merge_store_extra(store->albums, el.item().id_proto(), el.extra());

            for (qsizetype i = 0; i < el.songExtras().size(); i++) {
                auto id = el.songs().at(i).id_proto();
                merge_store_extra(store->songs, id, el.songExtras().at(i));
            }
        });
        co_return;
    });
}
} // namespace qcm

#include <Qcm/query/moc_album_query.cpp>