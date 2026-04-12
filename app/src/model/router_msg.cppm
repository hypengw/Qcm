module;
#include "Qcm/macro.hpp"
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/model/router_msg.moc"
#endif

export module qcm:model.router_msg;
export import qt;

namespace qcm
{
namespace model
{

export class RouteMsg {
    Q_GADGET
    QML_VALUE_TYPE(rmsg)
    QML_STRUCTURED_VALUE
public:
    GADGET_PROPERTY_DEF(QString, dst, dst)
    GADGET_PROPERTY_DEF(QVariantMap, props, props)
};

} // namespace model
} // namespace qcm