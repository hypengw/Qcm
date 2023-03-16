#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/album_sub.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class AlbumSub : public QObject {
    Q_OBJECT
public:
    AlbumSub(QObject* parent = nullptr): QObject(parent) {}

    using out_type = ncm::api_model::AlbumSub;

    void handle_output(const out_type&, const auto&) {}
};
static_assert(modelable<AlbumSub, ncm::api::AlbumSub>);

} // namespace model

using AlbumSubQuerier_base = ApiQuerier<ncm::api::AlbumSub, model::AlbumSub>;
class AlbumSubQuerier : public AlbumSubQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumSubQuerier(QObject* parent = nullptr): AlbumSubQuerier_base(parent) {}

    FORWARD_PROPERTY(model::AlbumId, itemId, id)
    FORWARD_PROPERTY(bool, sub, sub)
};
} // namespace qcm
