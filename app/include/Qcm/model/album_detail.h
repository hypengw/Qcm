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
        CONVERT_PROPERTY(o.m_name, in.album.name);
        CONVERT_PROPERTY(o.m_picUrl, in.album.picUrl);
        CONVERT_PROPERTY(o.m_description, in.album.description.value_or(""));
        CONVERT_PROPERTY(o.m_company, in.album.company.value_or(""));
        CONVERT_PROPERTY(o.m_type, in.album.type);
        CONVERT_PROPERTY(o.m_subType, in.album.subType);
        CONVERT_PROPERTY(o.m_alias, in.album.alias);
        CONVERT_PROPERTY(o.m_artists, in.album.artists);
        CONVERT_PROPERTY(o.m_size, in.album.size);
        CONVERT_PROPERTY(o.m_publishTime, in.album.publishTime);
        CONVERT_PROPERTY(o.m_songs, in.songs);
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
