module;
#include "Qcm/query/play_query.moc.h"
module qcm;
import rstd;
import :query.play;
import :app;

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
    cancel();
    const auto revision = ++m_revision;
    if (! m_item_id.valid() || (m_item_id.type() != enums::ItemType::ItemSong &&
                                m_item_id.type() != enums::ItemType::ItemAlbum &&
                                m_item_id.type() != enums::ItemType::ItemMix)) {
        setError(tr("Unsupported playback identity"));
        setStatus(Status::Error);
        return;
    }
    if (m_item_id.type() == enums::ItemType::ItemSong) {
        setStatus(Status::Querying);
        auto self = QPointer<PlayQuery>(this);
        App::instance()->playqueue()->playSong(m_item_id, [self, revision](bool success) {
            if (! self || self->m_revision != revision) return;
            if (! success) self->setError(tr("Playback request failed"));
            self->setStatus(success ? Status::Finished : Status::Error);
        });
    } else {
        setStatus(Status::Querying);
        auto backend = App::instance()->backend();
        auto req     = msg::GetSongIdsReq {};
        auto self    = QWatcher { this };

        {
            msg::filter::SongFilter filter;
            if (m_item_id.type() == enums::ItemType::ItemAlbum) {
                msg::filter::AlbumIdFilter sub_filter;
                sub_filter.setValue(*m_item_id.toAlbumId());
                filter.setAlbumIdFilter(sub_filter);
                req.setSort(msg::model::SongSortGadget::SongSort::SONG_SORT_TRACK_NUMBER);
                req.setAsc(true);
            } else if (m_item_id.type() == enums::ItemType::ItemMix) {
                msg::filter::MixIdFilter sub_filter;
                sub_filter.setValue(*m_item_id.toMixId());
                filter.setMixIdFilter(sub_filter);
            }
            req.setFilters({ filter });
        }

        spawn([self, backend, req, revision] mutable -> task<void> {
            auto rsp = co_await backend->send(std::move(req));
            if (! co_await QAsyncResult::qexecutor()) co_return;
            if (! self || self->m_revision != revision) co_return;
            if (rsp) {
                std::vector<model::ItemId> ids;
                for (auto id : rsp->ids().values()) {
                    ids.push_back(model::ItemId(enums::ItemType::ItemSong, qint64(id)));
                }
                App::instance()->playqueue()->replaceSongs(ids, [self, revision](bool success) {
                    if (! self || self->m_revision != revision) return;
                    if (! success) self->setError(tr("Playback request failed"));
                    self->setStatus(success ? Status::Finished : Status::Error);
                });
            } else {
                self->setError(QString::fromStdString(rsp.unwrap_err().message));
                self->setStatus(Status::Error);
            }
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
    cancel();
    const auto revision = ++m_revision;
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

    spawn([self, backend, req, revision] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || self->m_revision != revision) co_return;
        if (rsp) {
            std::vector<model::ItemId> ids;
            for (auto id : rsp->ids().values()) {
                ids.push_back(model::ItemId(enums::ItemType::ItemSong, qint64(id)));
            }
            App::instance()->playqueue()->replaceSongs(ids, [self, revision](bool success) {
                if (! self || self->m_revision != revision) return;
                if (! success) self->setError(tr("Playback request failed"));
                self->setStatus(success ? Status::Finished : Status::Error);
            });
        } else {
            self->setError(QString::fromStdString(rsp.unwrap_err().message));
            self->setStatus(Status::Error);
        }
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
    auto app       = App::instance();
    auto backend   = app->backend();
    auto req       = msg::GetRadioQueuesReq {};
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

} // namespace qcm

#include "Qcm/query/play_query.moc.cpp"
