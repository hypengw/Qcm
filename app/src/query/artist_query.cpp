module;
#include "Qcm/query/artist_query.moc.h"
module qcm;
import :query.artist;

namespace qcm
{

ArtistsQuery::ArtistsQuery(QObject* parent): QueryList(parent) {
    auto app = App::instance();
    this->tdata()->set_store(this->tdata(), AppStore::instance()->artists);
    connectSyncFinished(this);
    this->connect_requet_reload(&LibraryStatus::activedIdsChanged, app->libraryStatus());
}
void ArtistsQuery::reload() {
    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetArtistsReq {};
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
        self->inspect_set(rsp, [self](msg::GetArtistsRsp& el) {
            auto t = self->tdata();
            t->setHasMore(false);
            t->syncIds(el.ids());
            t->setHasMore(el.hasMore());
        });
        co_return;
    });
}
auto ArtistsQuery::filters() const -> const QList<msg::filter::ArtistFilter>& { return m_filters; }
void ArtistsQuery::setFilters(const QList<msg::filter::ArtistFilter>& f) {
    m_filters = f;
    invalidateRequests();
    filtersChanged();
}

auto ArtistsQuery::filterLogics() const -> const QList<msg::filter::FilterLogic>& {
    return m_filter_logics;
}
void ArtistsQuery::setFilterLogics(const QList<msg::filter::FilterLogic>& v) {
    m_filter_logics = v;
    invalidateRequests();
    filterLogicsChanged();
}

void ArtistsQuery::fetchMore(qint32) {
    if (noMore() || querying()) return;

    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetArtistsReq {};
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
        self->inspect_set(rsp, [self, offset](msg::GetArtistsRsp& el) {
            self->tdata()->appendIds(el.ids());
            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

AlbumArtistsQuery::AlbumArtistsQuery(QObject* parent): QueryList(parent) {
    auto app = App::instance();
    this->tdata()->set_store(this->tdata(), AppStore::instance()->artists);
    connectSyncFinished(this);
    this->connect_requet_reload(&LibraryStatus::activedIdsChanged, app->libraryStatus());
}
void AlbumArtistsQuery::reload() {
    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetAlbumArtistsReq {};
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
        self->inspect_set(rsp, [self](msg::GetAlbumArtistsRsp& el) {
            auto t = self->tdata();
            t->setHasMore(false);
            t->syncIds(el.ids());
            t->setHasMore(el.hasMore());
        });
        co_return;
    });
}
auto AlbumArtistsQuery::filters() const -> const QList<msg::filter::ArtistFilter>& {
    return m_filters;
}
void AlbumArtistsQuery::setFilters(const QList<msg::filter::ArtistFilter>& f) {
    m_filters = f;
    invalidateRequests();
    filtersChanged();
}

auto AlbumArtistsQuery::filterLogics() const -> const QList<msg::filter::FilterLogic>& {
    return m_filter_logics;
}
void AlbumArtistsQuery::setFilterLogics(const QList<msg::filter::FilterLogic>& v) {
    m_filter_logics = v;
    invalidateRequests();
    filterLogicsChanged();
}

void AlbumArtistsQuery::fetchMore(qint32) {
    if (noMore() || querying()) return;

    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetAlbumArtistsReq {};
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
        self->inspect_set(rsp, [self, offset](msg::GetAlbumArtistsRsp& el) {
            self->tdata()->appendIds(el.ids());
            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

ArtistQuery::ArtistQuery(QObject* parent): Query(parent) { connectSyncFinished(this); }
auto ArtistQuery::itemId() const -> model::ItemId { return m_item_id; }
void ArtistQuery::setItemId(model::ItemId in) {
    if (ycore::cmp_set(m_item_id, in)) {
        invalidateRequests();
        itemIdChanged();
    }
}

void ArtistQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetArtistReq {};
    req.setId_proto(m_item_id.id());
    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self](msg::GetArtistRsp& el) {
            self->tdata()->setItemId(el.id_proto());
        });
        co_return;
    });
}

ArtistAlbumQuery::ArtistAlbumQuery(QObject* parent): QueryList(parent) {
    connectSyncFinished(this);
    this->tdata()->set_store(this->tdata(), AppStore::instance()->albums);
}

auto ArtistAlbumQuery::itemId() const -> model::ItemId { return m_item_id; }
void ArtistAlbumQuery::setItemId(model::ItemId in) {
    if (ycore::cmp_set(m_item_id, in)) {
        invalidateRequests();
        itemIdChanged();
    }
}

void ArtistAlbumQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetArtistAlbumReq {};
    initReqForReload(req);
    req.setId_proto(m_item_id.id());

    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self](msg::GetArtistAlbumRsp& el) {
            self->tdata()->resetIds(el.ids());

            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

void ArtistAlbumQuery::fetchMore(qint32) {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetArtistAlbumReq {};
    initReqForFetchMore(req);
    req.setId_proto(m_item_id.id());

    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto offset = req.page();
        auto rsp    = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self, offset](msg::GetArtistAlbumRsp& el) {
            self->tdata()->appendIds(el.ids());

            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

} // namespace qcm

#include "Qcm/query/artist_query.moc.cpp"
