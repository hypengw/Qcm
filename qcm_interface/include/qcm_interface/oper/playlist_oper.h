#pragma once

#include "qcm_interface/oper/oper.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{
struct Playlist;
}

namespace qcm::oper
{
using ItemId = model::ItemId;

struct QCM_INTERFACE_API PlaylistOper : Oper<model::Playlist> {
    using Oper<model::Playlist>::Oper;

    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, name, name)
    OPER_PROPERTY(QString, picUrl, picUrl)
    OPER_PROPERTY(QString, description, description)
    OPER_PROPERTY(QDateTime, updateTime, updateTime)
    OPER_PROPERTY(qint32, playCount, playCount)
    OPER_PROPERTY(qint32, trackCount, trackCount)
    OPER_PROPERTY(bool, subscribed, subscribed)
    OPER_PROPERTY(ItemId, userId, userId)
};

} // namespace qcm::oper