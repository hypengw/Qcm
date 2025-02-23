#pragma once

#include <QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/sql/model_sql.h"

namespace qcm::model
{

struct MixRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(qcm::model::ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    GADGET_PROPERTY_DEF(qint32, trackCount, trackCount)

    QCM_INTERFACE_API static auto sql() -> const ModelSql&;
    std::strong_ordering          operator<=>(const MixRefer&) const = default;
};

struct Mix : MixRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_VALUE_TYPE(t_playlist)
public:
    GADGET_PROPERTY_DEF(qint32, specialType, specialType)
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(QDateTime, createTime, createTime)
    GADGET_PROPERTY_DEF(QDateTime, updateTime, updateTime)
    GADGET_PROPERTY_DEF(qint32, playCount, playCount)
    GADGET_PROPERTY_DEF(qcm::model::ItemId, userId, userId)
    GADGET_PROPERTY_DEF(std::vector<QString>, tags, tags)

    QCM_INTERFACE_API static auto sql() -> const ModelSql&;
};

} // namespace qcm::model