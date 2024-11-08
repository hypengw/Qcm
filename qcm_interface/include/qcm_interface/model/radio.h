#pragma once

#include <QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/sql/model_sql.h"

namespace qcm::model
{

struct RadioRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    std::strong_ordering operator<=>(const RadioRefer&) const = default;

    static auto sql() -> const ModelSql&;
};

struct Radio : RadioRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
public:
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(qint32, programCount, programCount)
    GADGET_PROPERTY_DEF(QDateTime, createTime, createTime)

    static auto sql() -> const ModelSql&;
};

} // namespace qcm::model