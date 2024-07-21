#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/playlist_tracks.h"

#include "core/log.h"
#include "core/qlist_helper.h"

namespace qcm
{
namespace model
{
class PlaylistTracks : public QObject {
    Q_OBJECT
public:
    PlaylistTracks(QObject* parent = nullptr): QObject(parent) {}

    using out_type = ncm::api_model::PlaylistTracks;

    void handle_output(const out_type&, const auto&) {}
};
static_assert(modelable<PlaylistTracks, ncm::api::PlaylistTracks>);

} // namespace model

using PlaylistTracksQuerier_base = ApiQuerier<ncm::api::PlaylistTracks, model::PlaylistTracks>;
class PlaylistTracksQuerier : public PlaylistTracksQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    PlaylistTracksQuerier(QObject* parent = nullptr): PlaylistTracksQuerier_base(parent) {}

    enum Oper
    {
        Add = 0,
        Del = 1
    };
    Q_ENUM(Oper)

    FORWARD_PROPERTY_DECLARE(Oper, operation, op)
    FORWARD_PROPERTY(model::PlaylistId, playlistId, pid)
    FORWARD_PROPERTY(QList<model::SongId>, trackIds, trackIds)
};

} // namespace qcm
DEFINE_CONVERT(qcm::PlaylistTracksQuerier::Oper, ncm::params::PlaylistTracks::Oper) {
    out = static_cast<out_type>(in);
}
DEFINE_CONVERT(ncm::params::PlaylistTracks::Oper, qcm::PlaylistTracksQuerier::Oper) {
    out = static_cast<out_type>(in);
}

namespace qcm
{
FORWARD_PROPERTY_IMPL(PlaylistTracksQuerier, PlaylistTracksQuerier::Oper, operation, op)
}