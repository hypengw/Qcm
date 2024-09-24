#pragma once

#include <QObject>
#include "qcm_interface/model.h"

namespace qcm
{
namespace model
{

class QCM_INTERFACE_API RouteMsg : public Model<RouteMsg> {
    Q_GADGET
    DECLARE_MODEL()
public:
    RouteMsg();
    ~RouteMsg();
    DECLARE_PROPERTY(QUrl, url, WRITE)
    DECLARE_PROPERTY(QVariantMap, props, WRITE)
};

} // namespace model
} // namespace qcm
