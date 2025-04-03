#pragma once

#include <QObject>
#include "qcm_interface/model.h"

namespace qcm
{
namespace model
{

class QCM_INTERFACE_API RouteMsg {
    Q_GADGET
public:
    RouteMsg() {}
    ~RouteMsg() {}
    GADGET_PROPERTY_DEF(QUrl, url, url)
    GADGET_PROPERTY_DEF(QVariantMap, props, props)
};

} // namespace model
} // namespace qcm
