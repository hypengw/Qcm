#include "Qcm/query/play_query.hpp"
#include "Qcm/util/async.inl"

#include "Qcm/query/album_query.hpp"
#include "Qcm/action.hpp"

namespace qcm
{
namespace
{
auto createSubQuery(QObject* parent, model::ItemId id) -> Query* {
    switch (id.type()) {
    case enums::ItemType::ItemAlbum: {
        auto query = new AlbumQuery(parent);
        query->setItemId(id);
        QObject::connect(query, &Query::finished, query, [query]() {
            auto&                      list = *(query->tdata());
            std::vector<model::ItemId> ids;
            for (usize i = 0; i < list.size(); ++i) {
                auto& item = list.at(i);
                ids.push_back(item.itemId());
            }
            Action::instance()->switch_songs(ids);
        });
        return query;
    }
    default: {
        return nullptr;
    }
    }
}
} // namespace

PlayQuery::PlayQuery(QObject* parent): Query(parent), m_sub_query(nullptr) {}

void PlayQuery::setSubQuery(Query* sub_query) {
    if (m_sub_query == sub_query) {
        return;
    }

    if (m_sub_query) {
        m_sub_query->cancel();
        QObject::disconnect(m_sub_query);
        m_sub_query->deleteLater();
    }
    m_sub_query = sub_query;
    if (m_sub_query) {
        m_sub_query->setParent(this);
        connect(m_sub_query, &Query::dataChanged, this, [this]() {
            set_data(m_sub_query->data());
        });
        connect(m_sub_query, &Query::errorOccurred, this, &PlayQuery::setError);
        connect(m_sub_query, &Query::statusChanged, this, &PlayQuery::setStatus);
    }
}
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
        setStatus(Status::Finished);
        setSubQuery(nullptr);
        Action::instance()->play(m_item_id);
    } else {
        auto query = createSubQuery(this, m_item_id);
        if (! query) return;
        setStatus(Status::Querying);
        setSubQuery(query);
        query->reload();
    }
}
} // namespace qcm

#include "Qcm/query/moc_play_query.cpp"