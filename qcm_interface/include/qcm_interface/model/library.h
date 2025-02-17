#pragma once

#include <QtCore/QUuid>
#include <QtQml/QQmlEngine>
#include "qcm_interface/export.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/client.h"

namespace qcm::model
{

struct Library {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(qint64, libraryId, libraryId)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(qint64, providerId, providerId)
    GADGET_PROPERTY_DEF(QString, nativeId, nativeId)
};

} // namespace qcm::model

namespace qcm
{
QCM_INTERFACE_API auto library(i32 id) -> std::optional<model::Library*>;
QCM_INTERFACE_API auto library_client(i32 id) -> std::optional<Client>;
} // namespace qcm