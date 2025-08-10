#pragma once

#include <QtCore/QAbstractListModel>
#include <QtQml/QQmlEngine>
#include "Qcm/message/filter.qpb.h"
#include "kstore/qt/gadget_model.hpp"

namespace qcm
{

class FilterRuleModel : public kstore::QGadgetListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged FINAL)
public:
    FilterRuleModel(kstore::QListInterface* list, QObject* = nullptr);
    ~FilterRuleModel();

    Q_SIGNAL void apply();
    Q_SIGNAL void reset();

    Q_INVOKABLE QString toJson() const;
    Q_SLOT void         fromJson(const QString&);

    auto toJsonDocument() const -> QJsonDocument;
    void fromJsonDocument(const QJsonDocument&);

    auto          dirty() const noexcept -> bool { return m_dirty; }
    void          setDirty(bool v);
    Q_SIGNAL void dirtyChanged();

private:
    virtual void fromVariantlist(const QVariantList& v) = 0;

    Q_SLOT void markDirty();
    bool        m_dirty;
};

class AlbumFilterRuleModel
    : public FilterRuleModel,
      public kstore::QMetaListModelCRTP<msg::filter::AlbumFilter, AlbumFilterRuleModel,
                                        kstore::ListStoreType::Vector> {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumFilterRuleModel(QObject* = nullptr);
    ~AlbumFilterRuleModel();

    void fromVariantlist(const QVariantList& v) override;
};

} // namespace qcm