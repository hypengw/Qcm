#include "Qcm/query/play_query.hpp"
#include "Qcm/util/async.inl"

#include "Qcm/query/album_query.hpp"
#include "Qcm/action.hpp"
#include "Qcm/backend.hpp"
#include "Qcm/app.hpp"

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
            co_return;
        });
    }
}
} // namespace qcm

#include "Qcm/query/moc_play_query.cpp"