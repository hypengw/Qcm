#pragma once

#include "qcm_interface/oper/oper.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{
struct ThirdUser;
}

namespace qcm::oper
{
using ItemId = model::ItemId;

struct QCM_INTERFACE_API ThirdUserOper : Oper<model::ThirdUser> {
    using Oper<model::ThirdUser>::Oper;

    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, name, name)
    OPER_PROPERTY(QString, picUrl, picUrl)
};

} // namespace qcm::oper