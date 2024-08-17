#pragma once

#include <QtCore/QAbstractListModel>
#include <QtQml/QQmlEngine>

#include "qcm_interface/api.h"

namespace qcm::qml_ncm
{

class CloudUploadModel : public QAbstractListModel {
    Q_OBJECT
public:
    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct Item {};
    std::vector<Item> m_items;
};

class CloudUploadApi : public ApiQuerierBase {
    Q_OBJECT
    QML_ELEMENT
public:
    auto data() const -> QObject* override;
    void reload() override;

public Q_SLOTS:
    void upload(const QUrl&);

private:
    auto upload_impl() -> asio::awaitable<void>;

    CloudUploadModel* m_data;
};
} // namespace qcm::qml_ncm