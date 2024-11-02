#pragma once

#include "qcm_interface/oper/oper.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{
struct Djradio;
}

namespace qcm::oper
{
using ItemId = model::ItemId;

struct QCM_INTERFACE_API DjradioOper : Oper<model::Djradio> {
    using Oper<model::Djradio>::Oper;

    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, name, name)
    OPER_PROPERTY(QString, picUrl, picUrl)
    OPER_PROPERTY(QString, description, description)
    OPER_PROPERTY(qint32, programCount, programCount)
    OPER_PROPERTY(QDateTime, createTime, createTime)
};

} // namespace qcm::oper