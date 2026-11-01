module;
#include "Qcm/query/mix_query.moc.h"
module qcm;
import :query.mix;

namespace qcm
{

MixesQuery::MixesQuery(QObject* parent): QueryList(parent) {
    auto app = App::instance();
    this->tdata()->set_store(this->tdata(), app->store()->mixes);
    connectSyncFinished(this);
    this->connect_requet_reload(&MixesQuery::filtersChanged, this);

    connect(Notifier::instance(), &Notifier::mixCreated, this, &MixesQuery::delayReload);
    connect(Notifier::instance(), &Notifier::mixDeleted, this, &MixesQuery::delayReload);
    connect(Notifier::instance(), &Notifier::mixLinked, this, &MixesQuery::delayReload);
}

void MixesQuery::reload() {
    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetMixsReq {};
    initReqForReload(req);
    req.setFilters(m_filters);
    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self](msg::GetMixsRsp& el) {
            auto t = self->tdata();
            t->setHasMore(false);
            t->resetIds(el.ids());
            t->setHasMore(el.hasMore());
        });
        co_return;
    });
}

void MixesQuery::fetchMore(qint32) {
    if (noMore() || querying()) return;

    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetMixsReq {};
    initReqForFetchMore(req);
    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto offset = req.page();
        auto rsp    = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self, offset](msg::GetMixsRsp& el) {
            self->tdata()->appendIds(el.ids());
            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

RemoteMixesQuery::RemoteMixesQuery(QObject* parent): QueryList(parent) {
    auto app = App::instance();
    this->tdata()->set_store(this->tdata(), app->store()->mixes);
    connectSyncFinished(this);
    this->connect_requet_reload(&RemoteMixesQuery::filtersChanged, this);
}
void RemoteMixesQuery::reload() {
    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetRemoteMixsReq {};
    initReqForReload(req);
    req.setFilters(m_filters);

    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self](msg::GetRemoteMixsRsp& el) {
            auto t = self->tdata();
            t->setHasMore(false);
            t->resetIds(el.ids());
            t->setHasMore(el.hasMore());
        });
        co_return;
    });
}
void RemoteMixesQuery::fetchMore(qint32) {}

MixQuery::MixQuery(QObject* parent): Query(parent) { connectSyncFinished(this); }

auto MixQuery::mixId() const -> msg::id::MixId { return m_mix_id; }

void MixQuery::setMixId(msg::id::MixId in) {
    if (ycore::cmp_set(m_mix_id, in)) {
        invalidateRequests();
        mixIdChanged();
    }
}

void MixQuery::reload() {
    if (! identity::valid(m_mix_id)) {
        setError(tr("Invalid mix ID"));
        setStatus(Status::Error);
        return;
    }
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetMixReq {};
    req.setId_proto(m_mix_id);
    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self](msg::GetMixRsp& el) {
            self->tdata()->setItemId(el.id_proto());
        });
        co_return;
    });
}

MixSongsQuery::MixSongsQuery(QObject* parent): QueryList(parent) {
    auto app = App::instance();
    this->tdata()->set_store(this->tdata(), app->store()->songs);
    connectSyncFinished(this);
    this->connect_requet_reload(&MixSongsQuery::mixIdChanged, this);
    this->setLimit(1000);
}

auto MixSongsQuery::mixId() const -> msg::id::MixId { return m_mix_id; }

void MixSongsQuery::setMixId(msg::id::MixId in) {
    if (ycore::cmp_set(m_mix_id, in)) {
        invalidateRequests();
        setOffset(0);
        mixIdChanged();
    }
}

void MixSongsQuery::reload() { load(false); }

void MixSongsQuery::refresh() {
    if (! querying()) load(true);
}

void MixSongsQuery::load(bool force_refresh) {
    if (! identity::valid(m_mix_id)) return;
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetMixSongsReq {};
    initReqForReload(req);
    req.setId_proto(m_mix_id);
    req.setForceRefresh(force_refresh);
    if (force_refresh) req.setPageSize(limit());

    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, req, revision, force_refresh] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self, force_refresh](msg::GetMixSongsRsp& el) {
            if (force_refresh) self->setOffset(0);
            self->tdata()->setMixId(el.mixId());

            self->tdata()->syncIds(el.ids());

            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

void MixSongsQuery::fetchMore(qint32) {
    if (! identity::valid(m_mix_id)) {
        setError(tr("Invalid mix ID"));
        setStatus(Status::Error);
        return;
    }
    if (noMore() || querying()) return;
    setStatus(Status::Querying);
    auto                backend = App::instance()->backend();
    msg::GetMixSongsReq request;
    initReqForFetchMore(request);
    request.setId_proto(m_mix_id);
    auto       self     = QWatcher { this };
    const auto revision = beginRequest();
    spawn([self, backend, request, revision]() mutable -> task<void> {
        const auto page     = request.page();
        auto       response = co_await backend->send(std::move(request));
        if (! co_await QAsyncResult::qexecutor() || ! self || ! self->isCurrentRequest(revision))
            co_return;
        self->inspect_set(response, [self, page](msg::GetMixSongsRsp& value) {
            self->tdata()->appendIds(value.ids());
            self->setOffset(page);
            self->tdata()->setHasMore(value.hasMore());
        });
    });
}

auto RemoteMixesQuery::filters() const -> const QList<msg::filter::RemoteMixFilter>& {
    return m_filters;
}
void RemoteMixesQuery::setFilters(const QList<msg::filter::RemoteMixFilter>& filters) {
    m_filters = filters;
    invalidateRequests();
    filtersChanged();
}
auto MixesQuery::filters() const -> const QList<msg::filter::MixFilter>& { return m_filters; }
void MixesQuery::setFilters(const QList<msg::filter::MixFilter>& filters) {
    m_filters = filters;
    invalidateRequests();
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
    auto self = QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto name = req.name();
        auto rsp  = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self) co_return;

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
auto DeleteMixQuery::ids() const -> msg::id::MixIds { return m_ids; }
void DeleteMixQuery::setIds(const msg::id::MixIds& ids) {
    m_ids = ids;
    idsChanged();
}
void DeleteMixQuery::reload() {
    if (! identity::valid(m_ids)) {
        setError(tr("Invalid mix IDs"));
        setStatus(Status::Error);
        return;
    }
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::DeleteMixReq {};
    req.setIds(m_ids);
    auto self = QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self) co_return;

        self->inspect_set(rsp, [self](auto&) {
            Notifier::instance()->mixDeleted();
        });

        co_return;
    });
}

LinkMixQuery::LinkMixQuery(QObject* parent): Query(parent) {}
auto LinkMixQuery::ids() const -> msg::id::MixIds { return m_ids; }
void LinkMixQuery::setIds(const msg::id::MixIds& ids) {
    m_ids = ids;
    idsChanged();
}
void LinkMixQuery::reload() {
    if (! identity::valid(m_ids)) {
        setError(tr("Invalid mix IDs"));
        setStatus(Status::Error);
        return;
    }
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::LinkMixReq {};
    req.setIds(m_ids);
    auto self = QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self) co_return;

        self->inspect_set(rsp, [self](auto&) {
            Notifier::instance()->mixLinked();
        });

        co_return;
    });
}

MixManipulateQuery::MixManipulateQuery(QObject* parent): Query(parent) {}
void MixManipulateQuery::reload() {
    if (! identity::valid(m_mix_id) || ! identity::valid(m_song_ids) ||
        ! identity::valid(m_album_ids)) {
        setError(tr("Invalid mix operation IDs"));
        setStatus(Status::Error);
        return;
    }
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::MixManipulateReq {};
    req.setId_proto(m_mix_id);
    req.setOper(m_oper);
    req.setSongIds(m_song_ids);
    req.setAlbumIds(m_album_ids);
    auto self = QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self) co_return;

        self->inspect_set(rsp, [self](auto&) {
        });

        co_return;
    });
}
auto MixManipulateQuery::mixId() const -> msg::id::MixId { return m_mix_id; }
void MixManipulateQuery::setMixId(const msg::id::MixId& id) {
    if (ycore::cmp_set(m_mix_id, id)) {
        mixIdChanged();
    }
}
auto MixManipulateQuery::songIds() const -> msg::id::SongIds { return m_song_ids; }
void MixManipulateQuery::setSongIds(const msg::id::SongIds& ids) {
    m_song_ids = ids;
    songIdsChanged();
}
auto MixManipulateQuery::albumIds() const -> msg::id::AlbumIds { return m_album_ids; }
void MixManipulateQuery::setAlbumIds(const msg::id::AlbumIds& ids) {
    m_album_ids = ids;
    albumIdsChanged();
}
auto MixManipulateQuery::oper() const -> msg::model::MixManipulateOperGadget::MixManipulateOper {
    return m_oper;
}
void MixManipulateQuery::setOper(msg::model::MixManipulateOperGadget::MixManipulateOper oper) {
    if (ycore::cmp_set(m_oper, oper)) {
        operChanged();
    }
}

} // namespace qcm

#include "Qcm/query/mix_query.moc.cpp"
