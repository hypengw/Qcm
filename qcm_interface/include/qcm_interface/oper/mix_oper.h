#pragma once

#include "qcm_interface/oper/oper.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{
struct Mix;
}

namespace qcm::oper
{
using ItemId = model::ItemId;

struct QCM_INTERFACE_API MixOper : Oper<model::Mix> {
    using Oper<model::Mix>::Oper;

    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, name, name)
    OPER_PROPERTY(QString, picUrl, picUrl)
    OPER_PROPERTY(qint64, libraryId, libraryId)
    OPER_PROPERTY(qint32, specialType, specialType)
    OPER_PROPERTY(QString, description, description)
    OPER_PROPERTY(QDateTime, createTime, createTime)
    OPER_PROPERTY(QDateTime, updateTime, updateTime)
    OPER_PROPERTY(qint32, playCount, playCount)
    OPER_PROPERTY(qint32, trackCount, trackCount)
    OPER_PROPERTY(bool, subscribed, subscribed)
    OPER_PROPERTY(ItemId, userId, userId)
    OPER_PROPERTY(std::vector<QString>, tags, tags)
};

} // namespace qcm::oper