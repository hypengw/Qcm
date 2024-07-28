#pragma once

#include <QObject>
#include "qcm_interface/model.h"

#include "core/log.h"

namespace qcm
{
namespace model
{

class QCM_INTERFACE_API AppInfo : public Model<AppInfo> {
    Q_GADGET
    DECLARE_MODEL(MT_COPY)
public:
    AppInfo();
    ~AppInfo();

    DECLARE_PROPERTY(QString, id)
    DECLARE_PROPERTY(QString, version)
};

} // namespace model
} // namespace qcm
