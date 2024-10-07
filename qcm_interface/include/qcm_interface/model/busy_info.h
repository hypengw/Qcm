#pragma once

#include <QObject>
#include "qcm_interface/model.h"

namespace qcm
{
namespace model
{

class QCM_INTERFACE_API BusyInfo : public Model<BusyInfo, QObject> {
    Q_OBJECT
    DECLARE_MODEL()
public:
    BusyInfo(QObject* parent = nullptr);
    ~BusyInfo();

    DECLARE_PROPERTY(bool, load_session, NOTIFY)
};

} // namespace model
} // namespace qcm
