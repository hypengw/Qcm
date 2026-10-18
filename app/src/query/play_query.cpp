module;
#include "Qcm/query/play_query.moc.h"
module qcm;
import :query.play;

namespace qcm
{

PlayQuery::PlayQuery(QObject* parent): Query(parent) {}

auto PlayQuery::itemId() const -> model::ItemId { return m_item_id; }
void PlayQuery::setItemId(model::ItemId v) {
    if (m_item_id == v) {
        return;
    }
    m_item_id = v;
    itemIdChanged(m_item_id);
}

void PlayQuery::reload() {
    if (m_item_id.type() == enums::ItemType::ItemSong) {
        cancel();
        setStatus(Status::Finished);
        Action::instance()->play(m_item_id);
    } else {
        setStatus(Status::Querying);
        auto backend = App::instance()->backend();
        auto req     = msg::GetSongIdsReq {};
        auto self    = QWatcher { this };

        {
            msg::filter::SongFilter filter;
            if (m_item_id.type() == enums::ItemType::ItemAlbum) {
                msg::filter::AlbumIdFilter sub_filter;
                sub_filter.setValue(m_item_id.id());
                filter.setAlbumIdFilter(sub_filter);
            } else if (m_item_id.type() == enums::ItemType::ItemMix) {
                msg::filter::MixIdFilter sub_filter;
                sub_filter.setValue(m_item_id.id());
                filter.setMixIdFilter(sub_filter);
            }
            req.setFilters({ filter });
        }

        spawn([self, backend, req] mutable -> task<void> {
            auto rsp = co_await backend->send(std::move(req));
            if (! co_await QAsyncResult::qexecutor()) co_return;
            if (! self) co_return;
            if (rsp) {
                std::vector<model::ItemId> ids;
                for (auto id : rsp->ids()) {
                    ids.push_back(model::ItemId(enums::ItemType::ItemSong, id));
                }
                Action::instance()->switch_songs(ids);
            }
            self->setStatus(Status::Finished);
            co_return;
        });
    }
}

PlayAllQuery::PlayAllQuery(QObject* parent)
    : Query(parent), m_sort(0), m_asc(true), m_album_sort(0), m_album_asc(true) {}

auto PlayAllQuery::filters() const -> const QList<msg::filter::AlbumFilter>& { return m_filters; }
void PlayAllQuery::setFilters(const QList<msg::filter::AlbumFilter>& in) {
    if (m_filters == in) {
        return;
    }
    m_filters = in;
    filtersChanged();
}

void PlayAllQuery::reload() {
    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetSongIdsReq {};
    auto self    = QWatcher { this };

    req.setSort(rstd::into(m_sort));
    req.setAlbumSort(rstd::into(m_album_sort));
    req.setAsc(m_asc);
    req.setAlbumAsc(m_album_asc);
    req.setAlbumFilters(m_filters);
    req.setLibraryIds(app->libraryStatus()->activedIds());

    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self) co_return;
        if (rsp) {
            std::vector<model::ItemId> ids;
            for (auto id : rsp->ids()) {
                ids.push_back(model::ItemId(enums::ItemType::ItemSong, id));
            }
            Action::instance()->switch_songs(ids);
        }
        self->setStatus(Status::Finished);
        co_return;
    });
}

auto PlayAllQuery::asc() const noexcept -> bool { return m_asc; }
auto PlayAllQuery::sort() const noexcept -> qint32 { return m_sort; }
auto PlayAllQuery::albumSort() const noexcept -> qint32 { return m_album_sort; }
auto PlayAllQuery::albumAsc() const noexcept -> bool { return m_album_asc; }

void PlayAllQuery::setAsc(bool v) {
    if (ycore::cmp_set(m_asc, v)) {
        ascChanged();
    }
}

void PlayAllQuery::setSort(qint32 v) {
    if (ycore::cmp_set(m_sort, v)) {
        sortChanged();
    }
}

void PlayAllQuery::setAlbumSort(qint32 v) {
    if (ycore::cmp_set(m_album_sort, v)) {
        albumSortChanged();
    }
}

void PlayAllQuery::setAlbumAsc(bool v) {
    if (ycore::cmp_set(m_album_asc, v)) {
        albumAscChanged();
    }
}

RadioQueuesQuery::RadioQueuesQuery(QObject* parent): QueryList(parent) {
    connectSyncFinished(this);
    auto providers = App::instance()->provider_status();
    m_provider_ids = providers->activedIds();
    connect(providers, &ProviderStatusModel::activedIdsChanged, this, [this, providers] {
        auto ids = providers->activedIds();
        if (m_provider_ids == ids) return;
        m_provider_ids = std::move(ids);
        ++m_revision;
        cancel();
        tdata()->resetModel(std::vector<model::RadioQueue> {});
        setStatus(Status::Uninitialized);
        delayReload();
    });
}

void RadioQueuesQuery::reload() {
    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetRadioQueuesReq {};
    m_provider_ids = app->provider_status()->activedIds();
    req.setProviderIds(m_provider_ids);
    auto self     = QWatcher { this };
    auto revision = ++m_revision;

    spawn([self, backend, req, revision] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || self->m_revision != revision) co_return;
        self->inspect_set(rsp, [self](auto& el) {
            auto t    = self->tdata();
            auto view = std::ranges::views::transform(el.queues(), [](auto&& el) {
                return model::RadioQueue(el);
            });
            t->resetModel(view);
        });
        co_return;
    });
}

QueueNextQuery::QueueNextQuery(QObject* parent): Query(parent), m_timeout(new QTimer(this)) {
    auto timer = static_cast<QTimer*>(m_timeout);
    timer->setSingleShot(true);
    timer->setInterval(170000);
    connect(timer, &QTimer::timeout, this, [this] {
        ++m_revision;
        cancel();
        m_transport_failure = true;
        setError(tr("Radio request timed out"));
        setStatus(Status::Error);
    });
}

void QueueNextQuery::reset() {
    static_cast<QTimer*>(m_timeout)->stop();
    ++m_revision;
    m_transport_failure = false;
    m_session_expired   = false;
    cancel();
    setStatus(Status::Uninitialized);
}
void QueueNextQuery::setRequest(const msg::OpenQueueReq& req) { m_request = req; }
void QueueNextQuery::setRequest(const msg::GetQueueNextReq& req) { m_request = req; }
auto QueueNextQuery::sessionExpired() const -> bool { return m_session_expired; }
auto QueueNextQuery::transportFailure() const -> bool { return m_transport_failure; }

void QueueNextQuery::reload() {
    static_cast<QTimer*>(m_timeout)->start();
    const auto revision = ++m_revision;
    setStatus(Status::Querying);
    m_session_expired   = false;
    m_transport_failure = false;
    auto backend        = App::instance()->backend();
    auto self           = QWatcher { this };
    std::visit(
        [&](auto req) {
            spawn([self, backend, req, revision] mutable -> task<void> {
                auto rsp = co_await backend->send(std::move(req));
                if (! co_await QAsyncResult::qexecutor()) co_return;
                if (! self || self->m_revision != revision) {
                    if constexpr (std::is_same_v<decltype(req), msg::OpenQueueReq>) {
                        if (rsp && ! rsp->session().id_proto().isEmpty()) {
                            msg::CloseQueueReq close;
                            close.setSessionId(rsp->session().id_proto());
                            co_await backend->send(std::move(close));
                        }
                    }
                    co_return;
                }
                static_cast<QTimer*>(self->m_timeout)->stop();
                if (rsp.is_err()) {
                    self->m_session_expired =
                        rsp.unwrap_err().code ==
                            static_cast<int>(msg::ErrorCodeGadget::ErrorCode::
                                                 ERROR_CODE_QUEUE_SESSION_EXPIRED) ||
                        rsp.unwrap_err().code ==
                            static_cast<int>(
                                msg::ErrorCodeGadget::ErrorCode::ERROR_CODE_QUEUE_SESSION_CONFLICT);
                    self->m_transport_failure = rsp.unwrap_err().code == -1;
                }
                self->inspect_set(rsp, [self](auto& el) {
                    msg::GetQueueNextRsp page;
                    page.setSongs(el.songs());
                    page.setExtras(el.extras());
                    page.setSession(el.session());
                    self->set_tdata(page);
                });
                co_return;
            });
        },
        m_request);
}

} // namespace qcm

#include "Qcm/query/play_query.moc.cpp"
