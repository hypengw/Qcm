module;
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/model/table_proxy.moc"
#endif

export module qcm:model.table_proxy;
export import qextra;

namespace qcm
{
export class TableProxyModel : public kstore::QTableProxyModel {
    Q_OBJECT
    QML_ELEMENT
public:
    TableProxyModel(QObject* parent = nullptr): kstore::QTableProxyModel(parent) {}
};

} // namespace qcm