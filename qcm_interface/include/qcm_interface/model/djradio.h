#pragma once

#include <QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{

struct DjradioRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    std::strong_ordering operator<=>(const DjradioRefer&) const = default;
};

struct Djradio : DjradioRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(qint32, programCount, programCount)

    static constexpr QStringView Select { uR"(
    djradio.itemId, 
    djradio.name, 
    djradio.picUrl, 
    djradio.description,
    djradio.programCount
)" };
};

} // namespace qcm::model