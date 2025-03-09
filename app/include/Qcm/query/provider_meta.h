#pragma once

#include <QtQml/QQmlEngine>
#include "Qcm/backend.h"
#include "qcm_interface/async.h"

namespace qcm::query
{

class ProviderMetasQuery : public QAsyncResultT<msg::GetProviderMetasRsp> {
    Q_OBJECT
    QML_ELEMENT
public:
    ProviderMetasQuery(QObject* parent = nullptr);
    void reload() override;
};
} // namespace qcm::query