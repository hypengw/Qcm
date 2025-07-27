#pragma once

#include <QtCore/QUrl>
#include <QtCore/QVariantMap>
#include <QtQml/QQmlEngine>
#include "Qcm/macro.hpp"

namespace qcm
{
namespace model
{

class RouteMsg {
    Q_GADGET
    QML_VALUE_TYPE(rmsg)
    QML_STRUCTURED_VALUE
public:

    GADGET_PROPERTY_DEF(QString, dst, dst)
    GADGET_PROPERTY_DEF(QVariantMap, props, props)
};

} // namespace model
} // namespace qcm
