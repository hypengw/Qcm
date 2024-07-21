#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
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

    READ_PROPERTY(ItemId, itemId, m_itemId, infoChanged)
    READ_PROPERTY(QString, name, m_name, infoChanged)
    READ_PROPERTY(QString, picUrl, m_picUrl, infoChanged)
    READ_PROPERTY(QString, description, m_description, infoChanged)
    READ_PROPERTY(QDateTime, updateTime, m_updateTime, infoChanged)
    READ_PROPERTY(qint32, playCount, m_playCount, infoChanged)
    READ_PROPERTY(ItemId, userId, m_userId, infoChanged)
    READ_PROPERTY(std::vector<Song>, songs, m_songs, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_itemId, in.playlist.id);
        convert(o.m_name, in.playlist.name);
        convert(o.m_picUrl, in.playlist.coverImgUrl);
        convert(o.m_description, in.playlist.description.value_or(""));
        if (in.playlist.updateTime) {
            convert(o.m_updateTime, in.playlist.updateTime.value());
        }
        convert(o.m_playCount, in.playlist.playCount);
        convert(o.m_userId, in.playlist.userId);
        convert(o.m_songs, in.playlist.tracks.value_or(std::vector<ncm::model::Song> {}));
        model::Playlist pl;
        convert(pl, in.playlist);
        for (auto& s : o.m_songs) {
            s.source = QVariant::fromValue(pl);
        }
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

    FORWARD_PROPERTY(model::ItemId, itemId, id)
};

} // namespace qcm
