#pragma once
#include <QQmlEngine>

namespace qcm
{
Q_NAMESPACE
QML_ELEMENT
    enum class ApiStatus {
        Uninitialized,
        Querying,
        Finished,
        Error
    };
Q_ENUM_NS(ApiStatus);
}