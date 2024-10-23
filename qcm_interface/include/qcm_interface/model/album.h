#pragma once

#include <QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{

struct Album {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    GADGET_PROPERTY_DEF(QDateTime, publishTime, publishTime)
    GADGET_PROPERTY_DEF(int, trackCount, trackCount)
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(QString, company, company)
    GADGET_PROPERTY_DEF(QString, type, type)

    // READ_PROPERTY(QString, subType, m_subType, infoChanged)
    // READ_PROPERTY(bool, paid, m_paid, infoChanged)
    // READ_PROPERTY(std::vector<QString>, alias, m_alias, infoChanged)

    std::strong_ordering operator<=>(const Album&) const = default;
};

} // namespace qcm::model