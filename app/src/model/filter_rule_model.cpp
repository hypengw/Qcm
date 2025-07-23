#include "Qcm/model/filter_rule_model.hpp"

namespace qcm
{
FilterRuleModel::FilterRuleModel(QObject* parent): QAbstractListModel(parent) {}
FilterRuleModel::~FilterRuleModel() {}
AlbumFilterRuleModel::AlbumFilterRuleModel(QObject* parent): FilterRuleModel(parent) {}
AlbumFilterRuleModel::~AlbumFilterRuleModel() {}

} // namespace qcm

#include "Qcm/model/moc_filter_rule_model.cpp"