#pragma once
#include "qcm_interface/model/session.h"
#include "service_qml_ncm/export.h"

namespace ncm::qml
{
class SERVICE_QML_NCM_API Session : public qcm::model::Session {
    Q_OBJECT
    QML_ELEMENT
public:
    Session(QObject* parent = nullptr);
    ~Session() override;
};
} // namespace ncm::qml