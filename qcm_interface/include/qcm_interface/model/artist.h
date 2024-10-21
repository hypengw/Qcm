#pragma once

#include <QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{

struct ArtistRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    std::strong_ordering operator<=>(const ArtistRefer&) const = default;
};

struct Artist : ArtistRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(QString, briefDesc, briefDesc)
    GADGET_PROPERTY_DEF(qint32, albumCount, albumCount)
    GADGET_PROPERTY_DEF(qint32, musicCount, musicCount)
    GADGET_PROPERTY_DEF(std::vector<QString>, alias, alias)
    // GADGET_PROPERTY_DEF(bool, followed, followed)
};

} // namespace qcm::model