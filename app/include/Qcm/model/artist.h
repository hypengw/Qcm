#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/artist.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class ArtistInfo : public QObject {
    Q_OBJECT
public:
    ArtistInfo(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::Artist;

    READ_PROPERTY(Artist, info, m_info, infoChanged)
    READ_PROPERTY(std::vector<Song>, hotSongs, m_hotSongs, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_info, in.artist);
        convert(o.m_hotSongs, in.hotSongs);
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<ArtistInfo, ncm::api::Artist>);

} // namespace model

using ArtistQuerier_base = ApiQuerier<ncm::api::Artist, model::ArtistInfo>;
class ArtistQuerier : public ArtistQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    ArtistQuerier(QObject* parent = nullptr): ArtistQuerier_base(parent) {}

    FORWARD_PROPERTY(model::ArtistId, itemId, id)
};
} // namespace qcm
