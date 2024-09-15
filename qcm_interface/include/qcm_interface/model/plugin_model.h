#pragma once

#include <QtCore/QAbstractListModel>
#include <QQmlEngine>
#include "core/core.h"
#include "qcm_interface/export.h"

namespace qcm
{

class QCM_INTERFACE_API PluginModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
public:
    PluginModel(QObject* parent = nullptr);
    ~PluginModel();

    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    class Private;
    C_DECLARE_PRIVATE(PluginModel, d_ptr);
};

} // namespace qcm