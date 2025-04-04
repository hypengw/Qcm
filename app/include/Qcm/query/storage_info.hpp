#pragma once

#include "Qcm/util/async.hpp"

namespace qcm::qml
{

class StorageInfo : public QObject {
    Q_OBJECT
    Q_PROPERTY(double total READ total NOTIFY totalChanged)
public:
    StorageInfo(QObject* parent);

    auto total() const -> double;
    void setTotal(double);

    Q_SIGNAL void totalChanged();

private:
    double m_total;
};

class StorageInfoQuerier : public QAsyncResultT<StorageInfo> {
    Q_OBJECT
    QML_ELEMENT
public:
    StorageInfoQuerier(QObject* parent = nullptr);
    void reload() override;
};

} // namespace qcm::qml