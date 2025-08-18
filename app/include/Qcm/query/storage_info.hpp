#pragma once

#include "Qcm/query/query.hpp"

namespace qcm::qml
{

class StorageInfo : public QObject {
    Q_OBJECT
    Q_PROPERTY(double total READ total NOTIFY totalChanged FINAL)
    Q_PROPERTY(double media READ media NOTIFY mediaChanged FINAL)
    Q_PROPERTY(double image READ image NOTIFY imageChanged FINAL)
    Q_PROPERTY(double database READ database NOTIFY databaseChanged FINAL)
public:
    StorageInfo(QObject* parent);

    auto total() const -> double;
    void setTotal(double);

    auto image() const -> double;
    void setImage(double v);

    auto media() const -> double;
    void setMedia(double v);

    auto database() const -> double;
    void setDatabase(double v);

    Q_SIGNAL void totalChanged();
    Q_SIGNAL void imageChanged();
    Q_SIGNAL void mediaChanged();
    Q_SIGNAL void databaseChanged();

    void updateTotal();

private:
    double m_total;
    double m_media;
    double m_image;
    double m_database;
};

class StorageInfoQuery : public Query, public QueryExtra<StorageInfo, StorageInfoQuery> {
    Q_OBJECT
    QML_ELEMENT
public:
    StorageInfoQuery(QObject* parent = nullptr);
    void reload() override;
};

} // namespace qcm::qml