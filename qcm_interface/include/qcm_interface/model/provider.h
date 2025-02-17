#pragma once

#include <QObject>
#include "qcm_interface/export.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/model.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/client.h"

namespace qcm::model
{

struct Provider {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(qint32, providerId, providerId)
    GADGET_PROPERTY_DEF(qcm::model::ItemId, userId, userId)
    GADGET_PROPERTY_DEF(QString, token, token)
    GADGET_PROPERTY_DEF(QString, nickname, nickname)
    GADGET_PROPERTY_DEF(QString, avatarUrl, avatarUrl)
    GADGET_PROPERTY_DEF(QUrl, server, server)
    GADGET_PROPERTY_DEF(QUrl, sessionFile, sessionFile)
    GADGET_PROPERTY_DEF(Extra, extra, extra)
};

} // namespace qcm::model

namespace qcm
{
QCM_INTERFACE_API auto provider(i32 id) -> std::optional<model::Provider*>;
QCM_INTERFACE_API auto provider_client(i32 id) -> std::optional<Client>;
} // namespace qcm