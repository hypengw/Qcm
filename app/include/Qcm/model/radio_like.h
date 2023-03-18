#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/radio_like.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class RadioLike : public QObject {
    Q_OBJECT
public:
    RadioLike(QObject* parent = nullptr): QObject(parent) {}

    using out_type = ncm::api_model::RadioLike;

    void handle_output(const out_type&, const auto&) {}
};
static_assert(modelable<RadioLike, ncm::api::RadioLike>);

} // namespace model

using RadioLikeQuerier_base = ApiQuerier<ncm::api::RadioLike, model::RadioLike>;
class RadioLikeQuerier : public RadioLikeQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    RadioLikeQuerier(QObject* parent = nullptr): RadioLikeQuerier_base(parent) {}

    FORWARD_PROPERTY(model::SongId, trackId, trackId)
    FORWARD_PROPERTY(bool, like, like)
};
}
