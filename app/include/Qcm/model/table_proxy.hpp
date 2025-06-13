#pragma once

#include <QtQml/QQmlEngine>
#include "meta_model/qtable_proxy_model.hpp"

namespace qcm
{
class TableProxyModel : public meta_model::QTableProxyModel {
    Q_OBJECT
    QML_ELEMENT
public:
    TableProxyModel(QObject* parent = nullptr): meta_model::QTableProxyModel(parent) {}
};


} // namespace qcm