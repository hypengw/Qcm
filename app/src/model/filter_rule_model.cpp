#include "Qcm/model/filter_rule_model.hpp"

namespace qcm
{
FilterRuleModel::FilterRuleModel(kstore::QListInterface* list, QObject* parent)
    : kstore::QGadgetListModel(list, parent), m_dirty(false) {
    connect(this, &QAbstractItemModel::dataChanged, this, &FilterRuleModel::markDirty);
    connect(this, &QAbstractItemModel::rowsInserted, this, &FilterRuleModel::markDirty);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &FilterRuleModel::markDirty);

    connect(this, &FilterRuleModel::apply, this, [this]() {
        setDirty(false);
    });
}
FilterRuleModel::~FilterRuleModel() {}

QString FilterRuleModel::toJson() const { return toJsonDocument().toJson(); }
void    FilterRuleModel::fromJson(const QString& j) {
    QJsonDocument doc = QJsonDocument::fromJson(j.toUtf8());
    fromJsonDocument(doc);
}

auto FilterRuleModel::toJsonDocument() const -> QJsonDocument { return {}; }
void FilterRuleModel::fromJsonDocument(const QJsonDocument&) {}
void FilterRuleModel::setDirty(bool v) {
    if (m_dirty != v) {
        m_dirty = v;
        dirtyChanged();
    }
}
void FilterRuleModel::markDirty() { setDirty(true); }

AlbumFilterRuleModel::AlbumFilterRuleModel(QObject* parent): FilterRuleModel(this, parent) {
    updateRoleNames(msg::filter::AlbumFilter::staticMetaObject, this);
}
AlbumFilterRuleModel::~AlbumFilterRuleModel() {}

} // namespace qcm

#include "Qcm/model/moc_filter_rule_model.cpp"