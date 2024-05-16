#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/album_detail.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class AlbumDetail : public QObject {
    Q_OBJECT
public:
    AlbumDetail(QObject* parent = nullptr): QObject(parent) {}

    READ_PROPERTY(QString, name, m_name, infoChanged)
    READ_PROPERTY(QString, picUrl, m_picUrl, infoChanged)
    READ_PROPERTY(QString, description, m_description, infoChanged)
    READ_PROPERTY(QString, company, m_company, infoChanged)
    READ_PROPERTY(QString, subType, m_subType, infoChanged)
    READ_PROPERTY(QString, type, m_type, infoChanged)
    READ_PROPERTY(bool, paid, m_paid, infoChanged)
    READ_PROPERTY(qint32, size, m_size, infoChanged)
    READ_PROPERTY(QDateTime, publishTime, m_publishTime, infoChanged)
    READ_PROPERTY(std::vector<QString>, alias, m_alias, infoChanged)
    READ_PROPERTY(std::vector<Artist>, artists, m_artists, infoChanged)
    READ_PROPERTY(std::vector<Song>, songs, m_songs, infoChanged)

    using out_type = ncm::api_model::AlbumDetail;

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_name, in.album.name);
        convert(o.m_picUrl, in.album.picUrl);
        convert(o.m_description, in.album.description.value_or(""));
        convert(o.m_company, in.album.company.value_or(""));
        convert(o.m_type, in.album.type);
        convert(o.m_subType, in.album.subType);
        convert(o.m_alias, in.album.alias);
        convert(o.m_artists, in.album.artists);
        convert(o.m_size, in.album.size);
        convert(o.m_publishTime, in.album.publishTime);
        convert(o.m_songs, in.songs);
        if (! in.album.picUrl.empty()) {
            for (auto& s : o.m_songs) {
                if (s.album.picUrl.isEmpty()) {
                    convert(s.album.picUrl, in.album.picUrl);
                }
            }
        }
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<AlbumDetail, ncm::api::AlbumDetail>);

} // namespace model

using AlbumDetailQuerier_base = ApiQuerier<ncm::api::AlbumDetail, model::AlbumDetail>;
class AlbumDetailQuerier : public AlbumDetailQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumDetailQuerier(QObject* parent = nullptr): AlbumDetailQuerier_base(parent) {}

    FORWARD_PROPERTY(model::AlbumId, itemId, id)
};

} // namespace qcm
