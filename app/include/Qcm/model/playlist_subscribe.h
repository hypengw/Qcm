#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/playlist_subscribe.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class PlaylistSubscribe : public QObject {
    Q_OBJECT
public:
    PlaylistSubscribe(QObject* parent = nullptr): QObject(parent) {}

    using out_type = ncm::api_model::PlaylistSubscribe;

    void handle_output(const out_type&, const auto&) {}
};
static_assert(modelable<PlaylistSubscribe, ncm::api::PlaylistSubscribe>);

} // namespace model

using PlaylistSubscribeQuerier_base =
    ApiQuerier<ncm::api::PlaylistSubscribe, model::PlaylistSubscribe>;
class PlaylistSubscribeQuerier : public PlaylistSubscribeQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    PlaylistSubscribeQuerier(QObject* parent = nullptr): PlaylistSubscribeQuerier_base(parent) {}

    FORWARD_PROPERTY(model::PlaylistId, itemId, id)
    FORWARD_PROPERTY(bool, sub, sub)
};
} // namespace qcm
