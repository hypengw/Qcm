#pragma once

#include "qcm_interface/oper/oper.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{
struct Album;
}

namespace qcm::oper
{
using ItemId = model::ItemId;

struct QCM_INTERFACE_API AlbumOper : Oper<model::Album> {
    using Oper<model::Album>::Oper;

    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, name, name)
    OPER_PROPERTY(QString, picUrl, picUrl)
    OPER_PROPERTY(QDateTime, publishTime, publishTime)
    OPER_PROPERTY(int, trackCount, trackCount)
};

} // namespace qcm::oper