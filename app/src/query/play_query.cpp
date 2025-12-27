#include "Qcm/query/play_query.hpp"
#include "Qcm/util/async.inl"

#include "Qcm/query/album_query.hpp"
#include "Qcm/action.hpp"
#include "Qcm/backend.hpp"
#include "Qcm/app.hpp"
#include "Qcm/status/provider_status.hpp"

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
        auto self    = helper::QWatcher { this };

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
            co_await qcm::qexecutor_switch();
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
    auto self    = helper::QWatcher { this };

    req.setSort(rstd::into(m_sort));
    req.setAlbumSort(rstd::into(m_album_sort));
    req.setAsc(m_asc);
    req.setAlbumAsc(m_album_asc);
    req.setAlbumFilters(m_filters);
    req.setLibraryIds(app->libraryStatus()->activedIds());

    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
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

} // namespace qcm

#include "Qcm/query/moc_play_query.cpp"