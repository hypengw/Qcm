#pragma once

#include <QQmlEngine>

#include "core/core.h"
#include "core/core.h"

namespace qcm
{
struct Duration {
    Q_GADGET
    QML_VALUE_TYPE(duration)
public:
    i64 value { 0 };
};
} // namespace qcm