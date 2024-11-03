#pragma once

#include <QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{

struct Program {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(QDateTime, duration, duration)
    GADGET_PROPERTY_DEF(QString, coverUrl, coverUrl)
    GADGET_PROPERTY_DEF(ItemId, songId, songId)
    GADGET_PROPERTY_DEF(QDateTime, createTime, createTime)
    GADGET_PROPERTY_DEF(qint32, serialNumber, serialNumber)
    GADGET_PROPERTY_DEF(ItemId, radioId, radioId)

    static constexpr QStringView Select { uR"(
    program.itemId,
    program.name,
    program.description,
    program.duration,
    program.coverUrl,
    program.songId,
    program.createTime,
    program.serialNumber,
    program.radioId
)" };

    std::strong_ordering operator<=>(const Program&) const = default;
};

} // namespace qcm::model