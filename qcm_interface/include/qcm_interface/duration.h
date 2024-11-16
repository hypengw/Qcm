#pragma once

#include <QQmlEngine>

#include "core/core.h"
#include "qcm_interface/export.h"

namespace qcm
{
struct Duration {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_VALUE_TYPE(t_duration)
public:
    i64 value { 0 };
};
} // namespace qcm