module;
#include "Qcm/model/filter_rule_model.moc.h"
module qcm;
import :model.filter_rule;

namespace qcm
{
FilterRuleModel::FilterRuleModel(kstore::QListInterface* list, QObject* parent)
    : kstore::QGadgetListModel(list, parent), m_dirty(false) {
    connect(this, &QAbstractItemModel::dataChanged, this, &FilterRuleModel::markDirty);
    connect(this, &QAbstractItemModel::rowsInserted, this, &FilterRuleModel::markDirty);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &FilterRuleModel::markDirty);
    connect(this, &FilterRuleModel::filterLogicsChanged, this, &FilterRuleModel::markDirty);

    connect(this, &FilterRuleModel::apply, this, [this]() {
        setDirty(false);
    });
    connect(this, &FilterRuleModel::reset, this, [this]() {
        setDirty(false);
    });
}

void FilterRuleModel::setFilterLogics(const QList<msg::filter::FilterLogic>& v) {
    m_filter_logics = v;
    filterLogicsChanged();
}

int FilterRuleModel::roleForName_(const QByteArray& name) const {
    const auto names = roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        if (it.value() == name) return it.key();
    }
    return -1;
}

int FilterRuleModel::groupOf_(const QVariant& v) const {
    const auto mt = v.metaType();
    const auto mo = mt.metaObject();
    if (! mo) return 0;
    const int idx = mo->indexOfProperty("group");
    if (idx < 0) return 0;
    return mo->property(idx).readOnGadget(v.constData()).toInt();
}

auto FilterRuleModel::orderedGroups_() const -> QList<int> {
    QList<int> groups;
    const int  role = roleForName_("group");
    if (role < 0) return groups;
    const int n = rowCount();
    for (int i = 0; i < n; ++i) {
        const int g = data(index(i, 0), role).toInt();
        if (groups.isEmpty() || groups.back() != g) groups.append(g);
    }
    return groups;
}

int FilterRuleModel::newGroupId() const {
    const int role = roleForName_("group");
    if (role < 0) return 0;
    int       max = -1;
    const int n   = rowCount();
    for (int i = 0; i < n; ++i) {
        const int g = data(index(i, 0), role).toInt();
        if (g > max) max = g;
    }
    return max + 1;
}

int FilterRuleModel::rowIndexInGroup(int row) const {
    const int role = roleForName_("group");
    if (role < 0 || row < 0 || row >= rowCount()) return 0;
    const int g     = data(index(row, 0), role).toInt();
    int       count = 0;
    for (int i = 0; i < row; ++i) {
        if (data(index(i, 0), role).toInt() == g) ++count;
    }
    return count;
}

int FilterRuleModel::rowCountInGroupOf(int row) const {
    const int role = roleForName_("group");
    if (role < 0 || row < 0 || row >= rowCount()) return 0;
    const int g = data(index(row, 0), role).toInt();
    return countInGroup(g);
}

int FilterRuleModel::countInGroup(int group) const {
    const int role = roleForName_("group");
    if (role < 0) return 0;
    int       n     = 0;
    const int total = rowCount();
    for (int i = 0; i < total; ++i) {
        if (data(index(i, 0), role).toInt() == group) ++n;
    }
    return n;
}

int FilterRuleModel::findInsertPosition(int group) const {
    const int role = roleForName_("group");
    if (role < 0) return rowCount();
    const int n   = rowCount();
    int       pos = 0;
    bool      seen = false;
    for (int i = 0; i < n; ++i) {
        const int g = data(index(i, 0), role).toInt();
        if (g <= group) {
            pos  = i + 1;
            seen = true;
        } else if (seen) {
            break;
        }
    }
    return pos;
}

int FilterRuleModel::sectionIndexForGroup(int group) const {
    const auto groups = orderedGroups_();
    return groups.indexOf(group);
}

int FilterRuleModel::findLogicAt(int section_index) const {
    if (section_index <= 0) return -1;
    const auto groups = orderedGroups_();
    if (section_index >= groups.size()) return -1;
    const int prev = groups[section_index - 1];
    const int curr = groups[section_index];
    for (int i = 0; i < m_filter_logics.size(); ++i) {
        const auto& e = m_filter_logics[i];
        if (e.groupA() == prev && e.groupB() == curr) return i;
    }
    return -1;
}

int FilterRuleModel::logicOpAt(int section_index) const {
    const int idx = findLogicAt(section_index);
    if (idx < 0) return -1;
    return static_cast<int>(m_filter_logics[idx].op());
}

void FilterRuleModel::setLogicOpAt(int section_index, int op) {
    int idx = findLogicAt(section_index);
    if (idx < 0) {
        const auto groups = orderedGroups_();
        if (section_index <= 0 || section_index >= groups.size()) return;
        msg::filter::FilterLogic e;
        e.setOp(static_cast<msg::filter::LogicOpGadget::LogicOp>(op));
        e.setGroupA(groups[section_index - 1]);
        e.setGroupB(groups[section_index]);
        m_filter_logics.append(e);
    } else {
        if (static_cast<int>(m_filter_logics[idx].op()) == op) return;
        m_filter_logics[idx].setOp(static_cast<msg::filter::LogicOpGadget::LogicOp>(op));
    }
    filterLogicsChanged();
}

void FilterRuleModel::appendRuleInGroup(int group) {
    const int role = roleForName_("group");
    const int row  = findInsertPosition(group);
    if (! insertRows(row, 1)) return;
    if (role >= 0) {
        QVariant v = item(row);
        const auto mt = v.metaType();
        const auto mo = mt.metaObject();
        if (mo) {
            const int idx = mo->indexOfProperty("group");
            if (idx >= 0) {
                mo->property(idx).writeOnGadget(v.data(), group);
                setItem(row, v);
            }
        }
    }
}

void FilterRuleModel::appendNewGroup() {
    const int new_id = newGroupId();
    int       prev_max = -1;
    if (rowCount() > 0) {
        const int role = roleForName_("group");
        if (role >= 0) {
            // since rules are sorted by group, last row's group is the max
            prev_max = data(index(rowCount() - 1, 0), role).toInt();
        }
    }
    appendRuleInGroup(new_id);
    if (prev_max >= 0) {
        msg::filter::FilterLogic e;
        e.setOp(msg::filter::LogicOpGadget::LogicOp::LOGIC_OP_AND);
        e.setGroupA(prev_max);
        e.setGroupB(new_id);
        m_filter_logics.append(e);
        filterLogicsChanged();
    }
}

void FilterRuleModel::deleteGroup(int group) {
    const int role = roleForName_("group");
    if (role < 0) return;
    for (int i = rowCount() - 1; i >= 0; --i) {
        if (data(index(i, 0), role).toInt() == group) removeRow(i);
    }
    bool changed = false;
    for (int i = m_filter_logics.size() - 1; i >= 0; --i) {
        const auto& e = m_filter_logics[i];
        if (e.groupA() == group || e.groupB() == group) {
            m_filter_logics.removeAt(i);
            changed = true;
        }
    }
    if (changed) filterLogicsChanged();
}

void FilterRuleModel::sortByGroup() {
    auto vs = items();
    std::stable_sort(vs.begin(), vs.end(), [this](const QVariant& a, const QVariant& b) {
        return groupOf_(a) < groupOf_(b);
    });
    fromVariantlist(vs);
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

    // Persist per-rule group explicitly. The proto gadget's `group` property
    // is also serialized via qvariant_to_josn, but we keep an explicit parallel
    // array so the round-trip cannot drop it.
    QJsonArray groups;
    const int  role = roleForName_("group");
    if (role >= 0) {
        const int n = rowCount();
        for (int i = 0; i < n; ++i) {
            groups.append(data(index(i, 0), role).toInt());
        }
    }
    obj.insert("groups", groups);

    QJsonArray logics;
    for (const auto& e : m_filter_logics) {
        QJsonObject jo;
        jo.insert("op", static_cast<int>(e.op()));
        jo.insert("group_a", static_cast<int>(e.groupA()));
        jo.insert("group_b", static_cast<int>(e.groupB()));
        logics.append(jo);
    }
    obj.insert("filter_logics", logics);

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

    // Re-apply per-rule group from the explicit "groups" array (if present),
    // so we don't depend on the protobuf gadget's reflection picking it up.
    if (obj.contains("groups")) {
        const auto groups = obj.value("groups").toArray();
        const int  role   = roleForName_("group");
        if (role >= 0) {
            const int n = std::min<int>(groups.size(), rowCount());
            for (int i = 0; i < n; ++i) {
                QVariant v = item(i);
                const auto mt = v.metaType();
                const auto mo = mt.metaObject();
                if (! mo) continue;
                const int idx = mo->indexOfProperty("group");
                if (idx < 0) continue;
                mo->property(idx).writeOnGadget(v.data(), groups.at(i).toInt());
                setItem(i, v);
            }
        }
    }

    QList<msg::filter::FilterLogic> logics;
    for (const auto& v : obj.value("filter_logics").toArray()) {
        const auto jo = v.toObject();
        msg::filter::FilterLogic e;
        e.setOp(static_cast<msg::filter::LogicOpGadget::LogicOp>(jo.value("op").toInt()));
        e.setGroupA(jo.value("group_a").toInt());
        e.setGroupB(jo.value("group_b").toInt());
        logics.append(e);
    }
    setFilterLogics(logics);

    sortByGroup();
}

void FilterRuleModel::setDirty(bool v) {
    if (m_dirty != v) {
        m_dirty = v;
        dirtyChanged();
    }
}
void FilterRuleModel::markDirty() { setDirty(true); }

AlbumFilterRuleModel::AlbumFilterRuleModel(QObject* parent): FilterRuleModel(this, parent) {
    updateRoleNames(msg::filter::AlbumFilter::staticMetaObject, this, {});
}
AlbumFilterRuleModel::~AlbumFilterRuleModel() {}

void AlbumFilterRuleModel::fromVariantlist(const QVariantList& v) {
    auto view = cppstd::views::transform(v, [](const QVariant& v) {
        return v.value<msg::filter::AlbumFilter>();
    });
    resetModel(view);
}
ArtistFilterRuleModel::ArtistFilterRuleModel(QObject* parent): FilterRuleModel(this, parent) {
    updateRoleNames(msg::filter::ArtistFilter::staticMetaObject, this, {});
}
ArtistFilterRuleModel::~ArtistFilterRuleModel() {}

void ArtistFilterRuleModel::fromVariantlist(const QVariantList& v) {
    auto view = cppstd::views::transform(v, [](const QVariant& v) {
        return v.value<msg::filter::ArtistFilter>();
    });
    resetModel(view);
}

MixFilterRuleModel::MixFilterRuleModel(QObject* parent): FilterRuleModel(this, parent) {
    updateRoleNames(msg::filter::MixFilter::staticMetaObject, this, {});
}
MixFilterRuleModel::~MixFilterRuleModel() {}

void MixFilterRuleModel::fromVariantlist(const QVariantList& v) {
    auto view = cppstd::views::transform(v, [](const QVariant& v) {
        return v.value<msg::filter::MixFilter>();
    });
    resetModel(view);
}

} // namespace qcm

#include "Qcm/model/filter_rule_model.moc.cpp"