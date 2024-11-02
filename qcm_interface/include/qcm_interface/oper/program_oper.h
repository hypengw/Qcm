#pragma once

#include "qcm_interface/oper/oper.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{
struct Program;
}

namespace qcm::oper
{
using ItemId = model::ItemId;

struct QCM_INTERFACE_API ProgramOper : Oper<model::Program> {
    using Oper<model::Program>::Oper;

    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, name, name)
    OPER_PROPERTY(QString, description, description)
    OPER_PROPERTY(QDateTime, duration, duration)
    OPER_PROPERTY(QString, coverUrl, coverUrl)
    OPER_PROPERTY(ItemId, songId, songId)
    OPER_PROPERTY(QDateTime, createTime, createTime)
    OPER_PROPERTY(qint32, serialNumber, serialNumber)
    OPER_PROPERTY(ItemId, radioId, radioId)
};

} // namespace qcm::oper