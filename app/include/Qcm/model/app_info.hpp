#pragma once

#include <QObject>
#include "qcm_interface/model.h"

namespace qcm
{
namespace model
{

class AppInfo {
    Q_GADGET
public:
    AppInfo() {}
    ~AppInfo() {}

    GADGET_PROPERTY_DEF(QString, id, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, version, version)
    GADGET_PROPERTY_DEF(QString, author, author)
    GADGET_PROPERTY_DEF(QString, summary, summary)
};

} // namespace model
} // namespace qcm
