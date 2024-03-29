#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/djradio_sub.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class DjradioSub : public QObject {
    Q_OBJECT
public:
    DjradioSub(QObject* parent = nullptr): QObject(parent) {}

    using out_type = ncm::api_model::DjradioSub;

    void handle_output(const out_type&, const auto&) {}
};
static_assert(modelable<DjradioSub, ncm::api::DjradioSub>);

} // namespace model

using DjradioSubQuerier_base = ApiQuerier<ncm::api::DjradioSub, model::DjradioSub>;
class DjradioSubQuerier : public DjradioSubQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    DjradioSubQuerier(QObject* parent = nullptr): DjradioSubQuerier_base(parent) {}

    FORWARD_PROPERTY(model::DjradioId, itemId, id)
    FORWARD_PROPERTY(bool, sub, sub)
};
} // namespace qcm
