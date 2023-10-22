#pragma once

#include <QSet>
#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
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

    Q_INVOKABLE bool contains(SongId id) const { return m_ids.contains(id.id); }
    Q_INVOKABLE void insert(SongId id) { m_ids.insert(id.id); }
    Q_INVOKABLE void remove(SongId id) { m_ids.remove(id.id); }

    using out_type = ncm::api_model::SongLike;
    void handle_output(const out_type& in, const auto&) {
        for (auto& id : in.ids) {
            m_ids.insert(convert_from<QString>(id));
        }
    }

private:
    QSet<QString> m_ids;
};
static_assert(modelable<SongLike, ncm::api::SongLike>);

} // namespace model

using SongLikeQuerier_base = ApiQuerier<ncm::api::SongLike, model::SongLike>;
class SongLikeQuerier : public SongLikeQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    SongLikeQuerier(QObject* parent = nullptr): SongLikeQuerier_base(parent) {}

    FORWARD_PROPERTY(model::UserId, uid, uid)
};
} // namespace qcm
