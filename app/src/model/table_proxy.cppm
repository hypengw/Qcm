module;
#include <QtQml/QQmlEngine>
#include "kstore/qt/qtable_proxy_model.hpp"

#include "Qcm/model/table_proxy.moc.h"
#ifdef Q_MOC_RUN
#    include "Qcm/model/table_proxy.moc"
#endif

export module qcm.model.table_proxy;
namespace qcm
{
export class TableProxyModel : public kstore::QTableProxyModel {
    Q_OBJECT
    QML_ELEMENT
public:
    TableProxyModel(QObject* parent = nullptr): kstore::QTableProxyModel(parent) {}
};

} // namespace qcm
module :private;

#include "Qcm/model/table_proxy.moc.cpp"