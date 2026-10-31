module;
#include "Qcm/model/library_item_source.moc.h"
#include "item_source_projection.hpp"
#include "qml_material/model/item_demand.hpp"
module qcm;
import :model.library_item_source;

namespace qcm::model
{
namespace
{
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
    const auto                   generation = ++m_projectionGeneration;
    QPointer<LibraryItemSource>  guard(this);
    QPointer<QAbstractItemModel> model    = m_model;
    const bool                   hadModel = bool(model);
    const auto                   current  = [&] {
        return guard && guard->m_projectionGeneration == generation && (! hadModel || model) &&
               guard->m_model == model;
    };
    const int count = model ? model->rowCount() : 0;
    if (! current()) return;
    if (count < 0) {
        reject(QStringLiteral("Invalid model row count"));
        return;
    }
    std::function<LibraryCardData(int)> read;
    const auto                          select = [&]<class T>(T* list) {
        if (! list) return false;
        read = [list](int index) {
            return cardData(list->at(index));
        };
        return true;
    };
    if (model && ! select(qobject_cast<ArtistListModel*>(model.data())) &&
        ! select(qobject_cast<AlbumListModel*>(model.data())) &&
        ! select(qobject_cast<MixListModel*>(model.data()))) {
        reject(QStringLiteral("Unsupported Library model"));
        return;
    }
    auto projection = detail::projectItemSource<LibraryCardData>(
        publicationBase(), count, first, last, QStringLiteral("library-card"), read, current);
    if (! projection) return;
    if (projection->valuesOnly)
        publishValues(std::move(projection->values));
    else
        publish(std::move(projection->snapshot), std::move(projection->changes), projection->reset);
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
