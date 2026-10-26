module;
#include "Qcm/model/library_item_source.moc.h"
#include "qml_material/model/item_source.hpp"
#include "qml_material/model/item_demand.hpp"
module qcm;
import :model.library_item_source;

namespace qcm::model
{
namespace
{
class LibrarySnapshot final : public qml_material::TypedItemDataSnapshot<LibraryCardData> {
public:
    LibrarySnapshot(QList<LibraryCardData> rows, QVector<quint64> versions)
        : m_rows(std::move(rows)), m_versions(std::move(versions)) {
        for (int i = 0; i < m_rows.size(); ++i) m_indexes.insert(m_rows[i].key, i);
    }
    int                          count() const override { return m_rows.size(); }
    qml_material::ItemDescriptor descriptorAt(int index) const override {
        return { m_rows.at(index).key, QStringLiteral("library-card"), m_versions.at(index) };
    }
    std::optional<int> indexOfKey(const QString& key) const override {
        auto found = m_indexes.constFind(key);
        return found == m_indexes.cend() ? std::nullopt : std::optional<int>(*found);
    }
    LibraryCardData itemAt(int index) const override { return m_rows.at(index); }

private:
    const QList<LibraryCardData> m_rows;
    const QVector<quint64>       m_versions;
    QHash<QString, int>          m_indexes;
};

LibraryCardData cardData(const Album& album) {
    LibraryCardData row;
    row.itemId     = item_id(album);
    row.key        = row.itemId.toString();
    row.name       = album.name();
    row.trackCount = album.trackCount();
    row.artists    = AppStore::instance()->artistsFor(row.itemId);
    return row;
}
LibraryCardData cardData(const Mix& mix) {
    LibraryCardData row;
    row.itemId     = item_id(mix);
    row.key        = row.itemId.toString();
    row.name       = mix.name();
    row.trackCount = mix.trackCount();
    return row;
}
LibraryCardData cardData(const Artist& artist) {
    LibraryCardData row;
    row.itemId     = item_id(artist);
    row.key        = row.itemId.toString();
    row.name       = artist.name();
    row.albumCount = artist.albumCount();
    row.musicCount = artist.musicCount();
    return row;
}
} // namespace

void LibraryItemSource::setModel(QAbstractItemModel* model) {
    if (m_model == model) return;
    for (const auto& connection : m_connections) disconnect(connection);
    m_connections.clear();
    m_model = model;
    ++m_generation;
    m_fetchQueued = false;
    m_requestedCount.reset();
    m_demand = {};
    if (model) {
        if (auto* list = qobject_cast<kstore::QMetaListModel*>(model))
            m_connections.append(
                connect(list, &kstore::QMetaListModel::hasMoreChanged, this, [this] {
                    m_requestedCount.reset();
                    requestMore(m_demand);
                }));
        const auto changed = [this] {
            rebuild();
        };
        m_connections.append({ connect(model, &QAbstractItemModel::rowsInserted, this, changed),
                               connect(model, &QAbstractItemModel::rowsRemoved, this, changed),
                               connect(model, &QAbstractItemModel::rowsMoved, this, changed),
                               connect(model, &QAbstractItemModel::modelReset, this, changed),
                               connect(model, &QAbstractItemModel::layoutChanged, this, changed),
                               connect(model,
                                       &QAbstractItemModel::dataChanged,
                                       this,
                                       [this](const QModelIndex& first, const QModelIndex& last) {
                                           rebuild(first.row(), last.row());
                                       }),
                               connect(model, &QObject::destroyed, this, [this] {
                                   m_model = nullptr;
                                   ++m_generation;
                                   m_demand = {};
                                   QPointer<LibraryItemSource> guard(this);
                                   rebuild();
                                   if (guard) emit modelChanged();
                               }) });
    }
    QPointer<LibraryItemSource> guard(this);
    rebuild();
    if (guard) emit modelChanged();
}
void LibraryItemSource::rebuild(int first, int last) {
    QList<LibraryCardData> rows;
    const auto             append = [&]<class T>(T* model) {
        if (! model) return false;
        rows.reserve(model->rowCount());
        for (int i = 0; i < model->rowCount(); ++i) rows.append(cardData(model->at(i)));
        return true;
    };
    if (m_model && ! append(qobject_cast<ArtistListModel*>(m_model.data())) &&
        ! append(qobject_cast<AlbumListModel*>(m_model.data())) &&
        ! append(qobject_cast<MixListModel*>(m_model.data()))) {
        reject(QStringLiteral("Unsupported Library model"));
        return;
    }
    const auto       before = publicationBase();
    QVector<quint64> versions;
    versions.reserve(rows.size());
    bool                              reset = rows.size() != before->count();
    QVector<qml_material::ItemChange> changes;
    for (int i = 0; i < rows.size(); ++i) {
        const auto& row   = rows[i];
        const int   old   = before->indexOfKey(row.key);
        const bool  dirty = first >= 0 && i >= first && i <= last;
        const bool  unchanged =
            old >= 0 && ! dirty && before->valueAt(old).value<LibraryCardData>() == row;
        versions.append(unchanged ? before->descriptorAt(old).contentRevision
                                  : before->revision + 1);
        if (i >= before->count() || before->descriptorAt(i).key != row.key) reset = true;
        if (! unchanged) {
            if (! changes.isEmpty() && changes.last().index + changes.last().count == i)
                ++changes.last().count;
            else
                changes.append(qml_material::ItemChange { qml_material::ItemChange::Update, i, 1 });
        }
    }
    publish(std::make_shared<LibrarySnapshot>(std::move(rows), std::move(versions)),
            reset ? QVector<qml_material::ItemChange>() : changes,
            reset);
}
void LibraryItemSource::requestMore(const qml_material::ItemDemand& demand) {
    if (! demand.valid() || demand.snapshot() != snapshot() || ! demand.reachesEnd()) return;
    m_demand = demand;
    if (! m_model || m_fetchQueued || m_requestedCount == count()) return;
    m_fetchQueued         = true;
    const auto generation = m_generation;
    QMetaObject::invokeMethod(
        this,
        [this, generation] {
            if (generation != m_generation) return;
            m_fetchQueued                       = false;
            const auto                   demand = m_demand;
            QPointer<LibraryItemSource>  guard(this);
            QPointer<QAbstractItemModel> model = m_model;
            if (! demand.valid() || demand.snapshot() != snapshot() || ! demand.reachesEnd() ||
                m_requestedCount == count())
                return;
            const bool canFetch = model && model->canFetchMore({});
            if (! guard || generation != m_generation || ! demand.valid() ||
                demand.snapshot() != snapshot())
                return;
            if (canFetch && model) {
                m_requestedCount = count();
                model->fetchMore({});
            }
        },
        Qt::QueuedConnection);
}
} // namespace qcm::model
#include "Qcm/model/library_item_source.moc.cpp"
