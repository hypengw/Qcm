#pragma once

#include <QtQml/QQmlEngine>
#include "kstore/qt/qtable_proxy_model.hpp"

namespace qcm
{
class TableProxyModel : public kstore::QTableProxyModel {
    Q_OBJECT
    QML_ELEMENT
public:
    TableProxyModel(QObject* parent = nullptr): kstore::QTableProxyModel(parent) {}
};


} // namespace qcm