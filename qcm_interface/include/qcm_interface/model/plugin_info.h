#pragma once

#include <QObject>
#include "qcm_interface/model.h"

namespace qcm
{
namespace model
{

class QCM_INTERFACE_API PluginInfo : public Model<PluginInfo> {
    Q_GADGET
    DECLARE_MODEL()
public:
    PluginInfo();
    ~PluginInfo();
    DECLARE_PROPERTY(QString, name, CONSTANT)
    DECLARE_PROPERTY(QString, fullname, CONSTANT)
    DECLARE_PROPERTY(QUrl, icon, CONSTANT)
};

} // namespace model
} // namespace qcm
