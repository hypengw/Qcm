#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/playlist_create.h"

#include "core/log.h"
#include "core/qlist_helper.h"

namespace qcm
{
namespace model
{
class PlaylistCreate : public QObject {
    Q_OBJECT
public:
    PlaylistCreate(QObject* parent = nullptr): QObject(parent) {}

    using out_type = ncm::api_model::PlaylistCreate;

    void handle_output(const out_type&, const auto&) {}
};
static_assert(modelable<PlaylistCreate, ncm::api::PlaylistCreate>);

} // namespace model

using PlaylistCreateQuerier_base = ApiQuerier<ncm::api::PlaylistCreate, model::PlaylistCreate>;
class PlaylistCreateQuerier : public PlaylistCreateQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    PlaylistCreateQuerier(QObject* parent = nullptr): PlaylistCreateQuerier_base(parent) {}

    FORWARD_PROPERTY(QString, name, name)
};
} // namespace qcm
