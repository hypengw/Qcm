#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/artist_sub.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class ArtistSub : public QObject {
    Q_OBJECT
public:
    ArtistSub(QObject* parent = nullptr): QObject(parent) {}

    using out_type = ncm::api_model::ArtistSub;

    void handle_output(const out_type&, const auto&) {}
};
static_assert(modelable<ArtistSub, ncm::api::ArtistSub>);

} // namespace model

using ArtistSubQuerier_base = ApiQuerier<ncm::api::ArtistSub, model::ArtistSub>;
class ArtistSubQuerier : public ArtistSubQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    ArtistSubQuerier(QObject* parent = nullptr): ArtistSubQuerier_base(parent) {}

    FORWARD_PROPERTY(model::ItemId, itemId, id)
    FORWARD_PROPERTY(bool, sub, sub)
};
} // namespace qcm
