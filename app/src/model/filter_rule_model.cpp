#include "Qcm/model/filter_rule_model.hpp"

namespace qcm
{
FilterRuleModel::FilterRuleModel(kstore::QListInterface* list, QObject* parent)
    : kstore::QGadgetListModel(list, parent) {}
FilterRuleModel::~FilterRuleModel() {}
AlbumFilterRuleModel::AlbumFilterRuleModel(QObject* parent): FilterRuleModel(this, parent) {
    updateRoleNames(msg::filter::AlbumFilter::staticMetaObject, this);
}
AlbumFilterRuleModel::~AlbumFilterRuleModel() {}

} // namespace qcm

#include "Qcm/model/moc_filter_rule_model.cpp"