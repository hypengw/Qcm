#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include "Qcm/macro.hpp"

namespace qcm
{
namespace model
{

class RouteMsg {
    Q_GADGET
public:
    RouteMsg() {}
    ~RouteMsg() {}
    GADGET_PROPERTY_DEF(QUrl, url, url)
    GADGET_PROPERTY_DEF(QVariantMap, props, props)
};

} // namespace model
} // namespace qcm
