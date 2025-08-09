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
public:
    FilterRuleModel(kstore::QListInterface* list, QObject* = nullptr);
    ~FilterRuleModel();

    Q_SIGNAL void apply();

private:
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
};

} // namespace qcm