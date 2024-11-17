#pragma once

#include <QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/model/third_user.h"

namespace qcm::model
{

class QCM_INTERFACE_API Comment {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(qcm::model::ItemId, itemId, id)
    GADGET_PROPERTY_DEF(ThirdUser, user, user)
    GADGET_PROPERTY_DEF(QString, content, content)
    GADGET_PROPERTY_DEF(QDateTime, time, time)
    GADGET_PROPERTY_DEF(bool, liked, liked)

    std::strong_ordering operator<=>(const Comment&) const = default;
};

} // namespace qcm::model