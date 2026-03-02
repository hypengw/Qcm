module;
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>
#include <QtQml/QQmlEngine>
#include "Qcm/macro.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/model/router_msg.moc"
#endif

export module qcm:model.router_msg;
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