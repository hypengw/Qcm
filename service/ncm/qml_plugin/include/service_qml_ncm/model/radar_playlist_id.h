#pragma once

#include <QtCore/QAbstractListModel>
#include <QtQml/QQmlEngine>

namespace qcm::qml_ncm
{

class RadarPlaylistIdModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
public:
    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};

} // namespace qcm::qml_ncm