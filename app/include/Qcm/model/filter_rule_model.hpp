#pragma once

#include <QtCore/QAbstractListModel>
#include <QtQml/QQmlEngine>
#include "Qcm/message/filter.qpb.h"

namespace qcm
{

class FilterRuleModel : public QAbstractListModel {
    Q_OBJECT
public:
    FilterRuleModel(QObject* = nullptr);
    ~FilterRuleModel();

private:
};

class AlbumFilterRuleModel : public FilterRuleModel {
    Q_OBJECT
    QML_ELEMENT
public:
    using value_t = msg::filter::AlbumFilter;

    AlbumFilterRuleModel(QObject* = nullptr);
    ~AlbumFilterRuleModel();

private:
    std::vector<value_t> m_items;
};
} // namespace qcm