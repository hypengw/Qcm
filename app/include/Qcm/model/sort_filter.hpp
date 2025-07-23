#pragma once
#include <QQmlEngine>
#include <QSortFilterProxyModel>

#include "kstore/qt/gadget_model.hpp"

namespace qcm
{

struct SortTypeItem {
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(qint32 type MEMBER type)
public:
    qint32  type { 0 };
    QString name;
};

class SortTypeModel
    : public kstore::QGadgetListModel,
      public kstore::QMetaListModelCRTP<SortTypeItem, SortTypeModel, kstore::ListStoreType::Vector> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qint32 currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged FINAL)
    Q_PROPERTY(
        qint32 currentType READ currentType WRITE setCurrentType NOTIFY currentTypeChanged FINAL)
    Q_PROPERTY(bool asc READ asc WRITE setAsc NOTIFY ascChanged FINAL)

public:
    SortTypeModel(QObject* parent);

    auto currentIndex() const -> qint32;
    auto currentType() const -> qint32;
    void setCurrentIndex(qint32);
    void setCurrentType(qint32);

    auto asc() const -> bool;
    void setAsc(bool);

    Q_SIGNAL void ascChanged();
    Q_SIGNAL void currentIndexChanged();
    Q_SIGNAL void currentTypeChanged();

private:
    qint32 m_current_idx;
    bool   m_asc;
};

class AlbumSortTypeModel : public SortTypeModel {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumSortTypeModel(QObject* parent = nullptr);
};

class ArtistSortTypeModel : public SortTypeModel {
    Q_OBJECT
    QML_ELEMENT
public:
    ArtistSortTypeModel(QObject* parent = nullptr);
};

class SongSortTypeModel : public SortTypeModel {
    Q_OBJECT
    QML_ELEMENT
public:
    SongSortTypeModel(QObject* parent = nullptr);
};

class SongSortFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qint32 sortType READ sortType WRITE setSortType NOTIFY sortTypeChanged FINAL)
    Q_PROPERTY(bool asc READ asc WRITE setAsc NOTIFY ascChanged FINAL)
public:
    SongSortFilterModel(QObject* parent = nullptr);
    auto sortType() const -> qint32;
    void setSortType(qint32);
    bool asc() const;
    void setAsc(bool);

    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

    Q_SIGNAL void ascChanged();
    Q_SIGNAL void sortTypeChanged();

private:
    Q_SLOT void freshSortType();
    Q_SLOT void freshSort();

    qint32 m_sort_type;
    bool   m_asc;
    qint32 m_disc_role;
};

} // namespace qcm