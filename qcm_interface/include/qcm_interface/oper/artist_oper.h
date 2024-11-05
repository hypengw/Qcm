#pragma once

#include "qcm_interface/oper/oper.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{
struct Artist;
}

namespace qcm::oper
{
using ItemId = model::ItemId;

struct QCM_INTERFACE_API ArtistOper : Oper<model::Artist> {
    using Oper<model::Artist>::Oper;

    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, name, name)
    OPER_PROPERTY(QString, picUrl, picUrl)
    OPER_PROPERTY(QString, description, description)
    OPER_PROPERTY(qint32, albumCount, albumCount)
    OPER_PROPERTY(qint32, musicCount, musicCount)
    OPER_PROPERTY(std::vector<QString>, alias, alias)
};

} // namespace qcm::oper