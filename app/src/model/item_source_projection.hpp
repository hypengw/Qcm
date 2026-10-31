#pragma once

#include "qml_material/model/item_source.hpp"
#include <QHash>
#include <QList>
#include <functional>
#include <optional>

namespace qcm::model::detail
{
template<class Row>
class ItemProjectionSnapshot final : public qml_material::TypedItemDataSnapshot<Row> {
public:
    ItemProjectionSnapshot(QList<Row> rows, QVector<quint64> versions, QString type)
        : m_rows(std::move(rows)), m_versions(std::move(versions)), m_type(std::move(type)) {
        for (int i = 0; i < m_rows.size(); ++i) m_indexes.insert(m_rows[i].key, i);
    }
    int                          count() const override { return m_rows.size(); }
    qml_material::ItemDescriptor descriptorAt(int index) const override {
        return { m_rows.at(index).key, m_type, m_versions.at(index) };
    }
    std::optional<int> indexOfKey(const QString& key) const override {
        const auto found = m_indexes.constFind(key);
        return found == m_indexes.cend() ? std::nullopt : std::optional<int>(*found);
    }
    Row itemAt(int index) const override { return m_rows.at(index); }

private:
    const QList<Row>       m_rows;
    const QVector<quint64> m_versions;
    const QString          m_type;
    QHash<QString, int>    m_indexes;
};

struct ItemSourceProjection {
    qml_material::ItemDataSnapshotPtr      snapshot;
    QVector<qml_material::ItemValueUpdate> values;
    QVector<qml_material::ItemChange>      changes;
    bool                                   valuesOnly = false;
    bool                                   reset      = false;
};

template<class Row, class Read, class Current>
std::optional<ItemSourceProjection>
projectItemSource(const qml_material::ItemSnapshotPtr& before, int count, int first, int last,
                  const QString& type, Read read, Current current) {
    const bool partial = first >= 0 && last >= first && last < count && count == before->count();
    const int  begin   = partial ? first : 0;
    const int  end     = partial ? last + 1 : count;
    QList<Row> rows;
    rows.reserve(end - begin);
    bool sameKeys = partial;
    for (int index = begin; index < end; ++index) {
        auto row = read(index);
        if (! current()) return {};
        if (partial) sameKeys &= row.key == before->descriptorAt(index).key;
        rows.append(std::move(row));
    }
    ItemSourceProjection result;
    if (sameKeys) {
        result.valuesOnly = true;
        result.values.reserve(rows.size());
        for (int i = 0; i < rows.size(); ++i)
            result.values.append({ begin + i, QVariant::fromValue(std::move(rows[i])) });
        return result;
    }
    if (partial) {
        QList<Row> complete;
        complete.reserve(count);
        for (int index = 0; index < count; ++index) {
            if (index >= begin && index < end)
                complete.append(std::move(rows[index - begin]));
            else {
                auto row = read(index);
                if (! current()) return {};
                complete.append(std::move(row));
            }
        }
        rows = std::move(complete);
    }
    QVector<quint64> versions;
    versions.reserve(count);
    result.reset = count != before->count();
    for (int index = 0; index < count; ++index) {
        const auto& row   = rows.at(index);
        const int   old   = before->indexOfKey(row.key);
        const bool  dirty = first >= 0 && index >= first && index <= last;
        const bool  unchanged =
            old >= 0 && ! dirty && before->valueAt(old).template value<Row>() == row;
        versions.append(unchanged ? before->descriptorAt(old).contentRevision
                                  : before->revision + 1);
        if (index >= before->count() || before->descriptorAt(index).key != row.key)
            result.reset = true;
        if (! unchanged) {
            if (! result.changes.isEmpty() &&
                result.changes.last().index + result.changes.last().count == index)
                ++result.changes.last().count;
            else
                result.changes.append({ qml_material::ItemChange::Update, index, 1 });
        }
    }
    result.snapshot =
        std::make_shared<ItemProjectionSnapshot<Row>>(std::move(rows), std::move(versions), type);
    if (result.reset) result.changes.clear();
    return result;
}
} // namespace qcm::model::detail
