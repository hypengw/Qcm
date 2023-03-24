#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/playlist_detail.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class PlaylistDetail : public QObject {
    Q_OBJECT
public:
    PlaylistDetail(QObject* parent = nullptr): QObject(parent) {}

    using out_type = ncm::api_model::PlaylistDetail;

    READ_PROPERTY(PlaylistId, itemId, m_itemId, infoChanged)
    READ_PROPERTY(QString, name, m_name, infoChanged)
    READ_PROPERTY(QString, picUrl, m_picUrl, infoChanged)
    READ_PROPERTY(QString, description, m_description, infoChanged)
    READ_PROPERTY(QDateTime, updateTime, m_updateTime, infoChanged)
    READ_PROPERTY(qint32, playCount, m_playCount, infoChanged)
    READ_PROPERTY(std::vector<Song>, songs, m_songs, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        CONVERT_PROPERTY(o.m_itemId, in.playlist.id);
        CONVERT_PROPERTY(o.m_name, in.playlist.name);
        CONVERT_PROPERTY(o.m_picUrl, in.playlist.coverImgUrl);
        CONVERT_PROPERTY(o.m_description, in.playlist.description.value_or(""));
        CONVERT_PROPERTY(o.m_updateTime, in.playlist.updateTime);
        CONVERT_PROPERTY(o.m_playCount, in.playlist.playCount);
        CONVERT_PROPERTY(o.m_songs, in.playlist.tracks.value_or(std::vector<ncm::model::Song> {}));
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<PlaylistDetail, ncm::api::PlaylistDetail>);

} // namespace model

using PlaylistDetailQuerier_base = ApiQuerier<ncm::api::PlaylistDetail, model::PlaylistDetail>;
class PlaylistDetailQuerier : public PlaylistDetailQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    PlaylistDetailQuerier(QObject* parent = nullptr): PlaylistDetailQuerier_base(parent) {}

    FORWARD_PROPERTY(model::PlaylistId, itemId, id)
};

} // namespace qcm
