#pragma once

#include <QtCore/QUuid>
#include <QtQml/QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/sql/model_sql.h"

namespace qcm::model
{

struct Song {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(qcm::model::ItemId, itemId, id)
    GADGET_PROPERTY_DEF(qint64, libraryId, libraryId)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(qcm::model::ItemId, albumId, albumId)
    GADGET_PROPERTY_DEF(qint32, trackNumber, trackNumber)
    GADGET_PROPERTY_DEF(QDateTime, duration, duration)
    GADGET_PROPERTY_DEF(bool, canPlay, canPlay)
    GADGET_PROPERTY_DEF(QString, coverUrl, coverUrl)
    GADGET_PROPERTY_DEF(QStringList, tags, tags)
    GADGET_PROPERTY_DEF(qreal, popularity, popularity)

    GADGET_PROPERTY_DEF(qcm::model::ItemId, sourceId, sourceId)

    QCM_INTERFACE_API static auto sql() -> const ModelSql&;

    bool operator==(const Song&) const = default;
};

} // namespace qcm::model