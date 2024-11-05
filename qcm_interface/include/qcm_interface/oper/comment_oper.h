#pragma once

#include "qcm_interface/oper/oper.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/oper/third_user_oper.h"

namespace qcm::model
{
struct Comment;
}

namespace qcm::oper
{
using ItemId = model::ItemId;

struct QCM_INTERFACE_API CommentOper : Oper<model::Comment> {
    using Oper<model::Comment>::Oper;

    OPER_PROPERTY(ItemId, itemId, id)
    OPER_PROPERTY(QString, content, content)
    OPER_PROPERTY_COPY(ThirdUserOper, user, user)
    OPER_PROPERTY(QDateTime, time, time)
    OPER_PROPERTY(bool, liked, liked)
};

} // namespace qcm::oper