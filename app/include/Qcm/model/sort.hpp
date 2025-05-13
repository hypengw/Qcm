#pragma once
#include <QQmlEngine>

#include "meta_model/qgadget_list_model.hpp"

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

class SortTypeModel : public meta_model::QGadgetListModel<SortTypeItem> {
    Q_OBJECT

    Q_PROPERTY(qint32 currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged FINAL)
    Q_PROPERTY(qint32 currentType READ currentType NOTIFY currentTypeChanged FINAL)
    Q_PROPERTY(bool asc READ asc WRITE setAsc NOTIFY ascChanged FINAL)
    using Base = meta_model::QGadgetListModel<SortTypeItem>;

public:
    SortTypeModel(QObject* parent);

    auto currentIndex() const -> qint32;
    auto currentType() const -> qint32;
    void setCurrentIndex(qint32);

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

} // namespace qcm