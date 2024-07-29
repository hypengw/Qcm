#pragma once

#include <QObject>
#include "qcm_interface/model.h"

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

    DECLARE_PROPERTY(QString, id, CONSTANT)
    DECLARE_PROPERTY(QString, name, CONSTANT)
    DECLARE_PROPERTY(QString, version, CONSTANT)
    DECLARE_PROPERTY(QString, author, CONSTANT)
    DECLARE_PROPERTY(QString, summary, CONSTANT)
};

} // namespace model
} // namespace qcm
