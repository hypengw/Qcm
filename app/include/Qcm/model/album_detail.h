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

class AlbumDetailInfo {
    Q_GADGET
public:
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QString, picUrl, picUrl)
    GATGET_PROPERTY(QString, description, description)
    GATGET_PROPERTY(QString, company, company)
    GATGET_PROPERTY(QString, subType, subType)
    GATGET_PROPERTY(QString, type, type)
    GATGET_PROPERTY(bool, paid, paid)
    GATGET_PROPERTY(qint32, size, size)
    GATGET_PROPERTY(QDateTime, publishTime, publishTime)
    GATGET_PROPERTY(std::vector<QString>, alias, alias)
    GATGET_PROPERTY(std::vector<Artist>, artists, artists)
    GATGET_PROPERTY(std::vector<Song>, songs, songs)
};

class AlbumDetail : public QObject {
    Q_OBJECT
public:
    AlbumDetail(QObject* parent = nullptr): QObject(parent) {}

    Q_PROPERTY(AlbumDetailInfo info READ info NOTIFY infoChanged)

    using out_type = ncm::api_model::AlbumDetail;

    void handle_output(const out_type& in, const auto&) {
        auto& o = m_info;
        CONVERT_PROPERTY(o.name, in.album.name);
        CONVERT_PROPERTY(o.picUrl, in.album.picUrl);
        CONVERT_PROPERTY(o.description, in.album.description.value_or(""));
        CONVERT_PROPERTY(o.company, in.album.company.value_or(""));
        CONVERT_PROPERTY(o.type, in.album.type);
        CONVERT_PROPERTY(o.subType, in.album.subType);
        CONVERT_PROPERTY(o.alias, in.album.alias);
        CONVERT_PROPERTY(o.artists, in.album.artists);
        CONVERT_PROPERTY(o.size, in.album.size);
        CONVERT_PROPERTY(o.publishTime, in.album.publishTime);
        CONVERT_PROPERTY(o.songs, in.songs);
        emit infoChanged();
    }

    AlbumDetailInfo info() const { return m_info; }

signals:
    void infoChanged();

private:
    AlbumDetailInfo m_info;
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
