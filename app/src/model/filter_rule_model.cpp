#include "Qcm/model/filter_rule_model.hpp"

#include "kstore/qt/meta_utils.hpp"
#include <QtCore/QJsonArray>

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
    connect(this, &FilterRuleModel::reset, this, [this]() {
        setDirty(false);
    });
}
FilterRuleModel::~FilterRuleModel() {}

QString FilterRuleModel::toJson() const {
    return toJsonDocument().toJson(QJsonDocument::JsonFormat::Compact);
}
void FilterRuleModel::fromJson(const QString& j) {
    QJsonDocument doc = QJsonDocument::fromJson(j.toUtf8());
    fromJsonDocument(doc);
}

auto FilterRuleModel::toJsonDocument() const -> QJsonDocument {
    auto list = kstore::qvariant_to_josn(this->items());
    auto obj  = QJsonObject();
    obj.insert("filters", list);
    auto doc = QJsonDocument();
    doc.setObject(obj);
    return doc;
}
void FilterRuleModel::fromJsonDocument(const QJsonDocument& doc) {
    if (doc.isNull() || ! doc.isObject()) {
        qWarning() << "Invalid JSON document for FilterRuleModel";
        return;
    }
    auto obj     = doc.object();
    auto filters = obj.value("filters").toArray();
    auto type    = m_oper->rawItemMeta()->metaType();

    QVariantList items;
    for (const auto& v : filters) {
        items.push_back(kstore::qvariant_from_josn(type, v));
    }
    fromVariantlist(items);
}

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

void AlbumFilterRuleModel::fromVariantlist(const QVariantList& v) {
    auto view = std::views::transform(v, [](const QVariant& v) {
        return v.value<msg::filter::AlbumFilter>();
    });
    resetModel(view);
}
ArtistFilterRuleModel::ArtistFilterRuleModel(QObject* parent): FilterRuleModel(this, parent) {
    updateRoleNames(msg::filter::ArtistFilter::staticMetaObject, this);
}
ArtistFilterRuleModel::~ArtistFilterRuleModel() {}

void ArtistFilterRuleModel::fromVariantlist(const QVariantList& v) {
    auto view = std::views::transform(v, [](const QVariant& v) {
        return v.value<msg::filter::ArtistFilter>();
    });
    resetModel(view);
}

MixFilterRuleModel::MixFilterRuleModel(QObject* parent): FilterRuleModel(this, parent) {
    updateRoleNames(msg::filter::MixFilter::staticMetaObject, this);
}
MixFilterRuleModel::~MixFilterRuleModel() {}

void MixFilterRuleModel::fromVariantlist(const QVariantList& v) {
    auto view = std::views::transform(v, [](const QVariant& v) {
        return v.value<msg::filter::MixFilter>();
    });
    resetModel(view);
}

} // namespace qcm

#include "Qcm/model/moc_filter_rule_model.cpp"