#pragma once

#include <QtCore/QUuid>
#include <QtQml/QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/sql/model_sql.h"

namespace qcm::model
{
struct AlbumRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(qcm::model::ItemId, itemId, id)
    GADGET_PROPERTY_DEF(qint64, libraryId, libraryId)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)

    std::strong_ordering operator<=>(const AlbumRefer&) const = default;

    QCM_INTERFACE_API static auto sql() -> const ModelSql&;
};

struct Album : AlbumRefer {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(QDateTime, publishTime, publishTime)
    GADGET_PROPERTY_DEF(int, trackCount, trackCount)
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(QString, company, company)
    GADGET_PROPERTY_DEF(QString, type, type)

    QCM_INTERFACE_API static auto sql() -> const ModelSql&;
};

} // namespace qcm::model