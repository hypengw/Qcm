#pragma once

#include "qcm_interface/oper/oper.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{
struct AlbumRefer;
struct Album;
} // namespace qcm::model

namespace qcm::oper
{
using ItemId = model::ItemId;

struct QCM_INTERFACE_API AlbumReferOper : Oper<model::AlbumRefer> {
    using Oper<model::AlbumRefer>::Oper;

    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, name, name)
    OPER_PROPERTY(QString, picUrl, picUrl)
    OPER_PROPERTY(qint64, libraryId, libraryId)
};

struct QCM_INTERFACE_API AlbumOper : Oper<model::Album> {
    using Oper<model::Album>::Oper;

    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, name, name)
    OPER_PROPERTY(QString, picUrl, picUrl)
    OPER_PROPERTY(qint64, libraryId, libraryId)
    OPER_PROPERTY(QDateTime, publishTime, publishTime)
    OPER_PROPERTY(qint32, trackCount, trackCount)
    OPER_PROPERTY(QString, description, description)
    OPER_PROPERTY(QString, company, company)
    OPER_PROPERTY(QString, type, type)
};

} // namespace qcm::oper