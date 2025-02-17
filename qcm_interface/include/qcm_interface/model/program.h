#pragma once

#include <QtCore/QUuid>
#include <QtQml/QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/sql/model_sql.h"

namespace qcm::model
{

struct Program {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(qcm::model::ItemId, itemId, id)
    GADGET_PROPERTY_DEF(qint64, libraryId, libraryId)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(QDateTime, duration, duration)
    GADGET_PROPERTY_DEF(QString, coverUrl, coverUrl)
    GADGET_PROPERTY_DEF(qcm::model::ItemId, songId, songId)
    GADGET_PROPERTY_DEF(QDateTime, createTime, createTime)
    GADGET_PROPERTY_DEF(qint32, serialNumber, serialNumber)
    GADGET_PROPERTY_DEF(qcm::model::ItemId, radioId, radioId)
    QCM_INTERFACE_API static auto sql() -> const ModelSql&;

    std::strong_ordering operator<=>(const Program&) const = default;
};

} // namespace qcm::model