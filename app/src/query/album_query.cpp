module;
#include "Qcm/query/album_query.moc.h"
module qcm;
import :query.album;

namespace qcm
{

AlbumsQuery::AlbumsQuery(QObject* parent): QueryList(parent) {
    auto app = App::instance();
    this->tdata()->set_store(this->tdata(), app->store()->albums);
    connectSyncFinished(this);
    this->connect_requet_reload(&LibraryStatus::activedIdsChanged, app->libraryStatus());
    this->connect_requet_reload(&AlbumsQuery::filtersChanged, this);
    this->connect_requet_reload(&AlbumsQuery::filterLogicsChanged, this);
}

auto AlbumsQuery::filters() const -> const QList<msg::filter::AlbumFilter>& { return m_filters; }
void AlbumsQuery::setFilters(const QList<msg::filter::AlbumFilter>& filters) {
    m_filters = filters;
    invalidateRequests();
    filtersChanged();
}

auto AlbumsQuery::filterLogics() const -> const QList<msg::filter::FilterLogic>& {
    return m_filter_logics;
}
void AlbumsQuery::setFilterLogics(const QList<msg::filter::FilterLogic>& v) {
    m_filter_logics = v;
    invalidateRequests();
    filterLogicsChanged();
}

void AlbumsQuery::reload() {
    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetAlbumsReq {};
    initReqForReload(req);
    req.setLibraryId(app->libraryStatus()->activedIds());
    req.setFilters(m_filters);
    req.setFilterLogics(m_filter_logics);

    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self](msg::GetAlbumsRsp& el) {
            auto t = self->tdata();
            t->setHasMore(false);
            t->syncIds(el.ids());

            t->setHasMore(el.hasMore());
        });
        co_return;
    });
}

void AlbumsQuery::fetchMore(qint32) {
    if (noMore() || querying()) return;

    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetAlbumsReq {};
    initReqForFetchMore(req);
    req.setLibraryId(app->libraryStatus()->activedIds());
    req.setFilters(m_filters);
    req.setFilterLogics(m_filter_logics);

    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto offset = req.page();
        auto rsp    = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self, offset](msg::GetAlbumsRsp& el) {
            self->tdata()->appendIds(el.ids());

            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

AlbumQuery::AlbumQuery(QObject* parent): QueryList(parent) {
    connectSyncFinished(this);
    this->tdata()->set_store(this->tdata(), App::instance()->store()->songs);
}
auto AlbumQuery::itemId() const -> model::ItemId { return m_item_id; }
void AlbumQuery::setItemId(model::ItemId in) {
    if (ycore::cmp_set(m_item_id, in)) {
        invalidateRequests();
        itemIdChanged();
    }
}

void AlbumQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetAlbumReq {};
    req.setId_proto(m_item_id.id());
    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self](msg::GetAlbumRsp& el) {
            auto store = App::instance()->store();
            auto t     = self->tdata();
            t->setAlbumId(el.id_proto());

            i64 disc = 1;
            for (auto id : el.songIds()) {
                if (auto song = store->songs.store_query(id))
                    disc = std::max<i64>(song->discNumber(), disc);
            }
            t->setDiscCount(disc);
            t->resetIds(el.songIds());
        });
        co_return;
    });
}

} // namespace qcm
#include "Qcm/query/album_query.moc.cpp"