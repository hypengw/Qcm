#pragma once

#include <QQuickImageProvider>
#include "service_qml_ncm/export.h"

namespace qcm::qml_ncm
{
SERVICE_QML_NCM_API auto create_ncm_imageprovider() -> QQuickImageProvider*;
} // namespace qcm::qml_ncm
