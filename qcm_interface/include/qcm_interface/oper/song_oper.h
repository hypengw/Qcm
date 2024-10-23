#pragma once

#include "qcm_interface/oper/oper.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{
struct Song;
}

namespace qcm::oper
{
using ItemId = model::ItemId;

struct QCM_INTERFACE_API SongOper : Oper<model::Song> {
    using Oper<model::Song>::Oper;
    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, name, name)
    OPER_PROPERTY(ItemId, albumId, albumId)
    OPER_PROPERTY(qint32, trackNumber, trackNumber)
    OPER_PROPERTY(QDateTime, duration, duration)
    OPER_PROPERTY(bool, canPlay, canPlay)
    OPER_PROPERTY(QString, coverUrl, coverUrl)
    OPER_PROPERTY(QStringList, tags, tags)
    OPER_PROPERTY(qreal, popularity, popularity)

    OPER_PROPERTY(QVariant, source, source)
    OPER_PROPERTY(ItemId, sourceId, sourceId)
};

} // namespace qcm::oper