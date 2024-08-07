#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/playlist_delete.h"

#include "core/log.h"
#include "core/qlist_helper.h"

namespace qcm
{
namespace model
{
class PlaylistDelete : public QObject {
    Q_OBJECT
public:
    PlaylistDelete(QObject* parent = nullptr): QObject(parent) {}

    using out_type = ncm::api_model::PlaylistDelete;

    void handle_output(const out_type&, const auto&) {}
};
static_assert(modelable<PlaylistDelete, ncm::api::PlaylistDelete>);

} // namespace model

using PlaylistDeleteQuerier_base = ApiQuerier<ncm::api::PlaylistDelete, model::PlaylistDelete>;
class PlaylistDeleteQuerier : public PlaylistDeleteQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    PlaylistDeleteQuerier(QObject* parent = nullptr): PlaylistDeleteQuerier_base(parent) {}

    FORWARD_PROPERTY(QList<model::ItemId>, itemIds, ids)
};
} // namespace qcm
