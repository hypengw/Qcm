#pragma once

#include <QtQml/QQmlEngine>
#include <QtCore/QAbstractListModel>
#include <QtCore/QIdentityProxyModel>
#include <QtCore/QSortFilterProxyModel>

import qcm;

namespace qcm
{
class QmlRegisterHelper : public QObject {
    Q_OBJECT
    QML_ELEMENT
};
} // namespace qcm