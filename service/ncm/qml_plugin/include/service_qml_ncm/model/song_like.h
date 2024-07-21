#pragma once

#include <unordered_set>
#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/song_like.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class SongLike : public QObject {
    Q_OBJECT
public:
    SongLike(QObject* parent = nullptr): QObject(parent) {}

    Q_INVOKABLE bool contains(ItemId id) const { return m_ids.contains(id); }
    Q_INVOKABLE void insert(ItemId id) { m_ids.insert(id); }
    Q_INVOKABLE void remove(ItemId id) { m_ids.erase(id); }

    using out_type = ncm::api_model::SongLike;
    void handle_output(const out_type& in, const auto&) {
        m_ids.clear();
        for (auto& id : in.ids) {
            m_ids.insert(ncm::to_ncm_id(ncm::model::IdType::Song, id));
        }
    }

private:
    std::unordered_set<ItemId> m_ids;
};
static_assert(modelable<SongLike, ncm::api::SongLike>);

} // namespace model

using SongLikeQuerier_base = ApiQuerier<ncm::api::SongLike, model::SongLike>;
class SongLikeQuerier : public SongLikeQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    SongLikeQuerier(QObject* parent = nullptr): SongLikeQuerier_base(parent) {}

    FORWARD_PROPERTY(model::ItemId, uid, uid)
};
} // namespace qcm
