module;
#include "Qcm/model/home_item_source.moc.h"
#include "item_source_projection.hpp"
module qcm;
import :model.home_item_source;

namespace qcm::model
{
namespace
{
HomeCardData cardData(const ForYouCard& card) {
    HomeCardData row;
    row.key      = kstore::ItemTrait<ForYouCard>::key(card);
    row.itemId   = item_id(card);
    row.coverId  = cover_id(card);
    row.name     = card.title();
    row.detail   = card_detail(card);
    row.badge    = card.badge();
    row.entryKey = card.entryKey();
    row.card     = card;
    if (card.hasRadio()) row.radio = card.radio();
    row.style      = int(card.style());
    row.coverKind  = int(card.coverKind());
    row.playAction = int(card.playAction());
    return row;
}
HomeCardData cardData(const RadioQueue& radio) {
    HomeCardData row;
    row.itemId = item_id(radio);
    row.key    = row.itemId.toString();
    row.name   = radio.name();
    row.detail = radio.description();
    row.radio  = radio;
    return row;
}
HomeCardData cardData(const Album& album) {
    HomeCardData row;
    row.itemId     = item_id(album);
    row.key        = row.itemId.toString();
    row.name       = album.name();
    row.trackCount = album.trackCount();
    row.artists    = AppStore::instance()->artistsFor(row.itemId);
    return row;
}
HomeCardData cardData(const Mix& mix) {
    HomeCardData row;
    row.itemId     = item_id(mix);
    row.key        = row.itemId.toString();
    row.name       = mix.name();
    row.trackCount = mix.trackCount();
    return row;
}
} // namespace

void HomeItemSource::setModel(QAbstractItemModel* model) {
    if (m_model == model) return;
    for (const auto& connection : m_connections) disconnect(connection);
    m_connections.clear();
    m_model = model;
    if (model) {
        const auto changed = [this] {
            rebuild();
        };
        m_connections = { connect(model, &QAbstractItemModel::rowsInserted, this, changed),
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
                              QPointer<HomeItemSource> guard(this);
                              rebuild();
                              if (guard) emit modelChanged();
                          }) };
    }
    QPointer<HomeItemSource> guard(this);
    rebuild();
    if (guard) emit modelChanged();
}
void HomeItemSource::rebuild(int first, int last) {
    const auto                   generation = ++m_projectionGeneration;
    QPointer<HomeItemSource>     guard(this);
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
    std::function<HomeCardData(int)> read;
    const auto                       select = [&]<class T>(T* list) {
        if (! list) return false;
        read = [list](int index) {
            return cardData(list->at(index));
        };
        return true;
    };
    if (model && ! select(qobject_cast<ForYouListModel*>(model.data())) &&
        ! select(qobject_cast<RadioQueueListModel*>(model.data())) &&
        ! select(qobject_cast<AlbumListModel*>(model.data())) &&
        ! select(qobject_cast<MixListModel*>(model.data()))) {
        reject(QStringLiteral("Unsupported Home model"));
        return;
    }
    auto projection = detail::projectItemSource<HomeCardData>(
        publicationBase(), count, first, last, QStringLiteral("home-card"), read, current);
    if (! projection) return;
    if (projection->valuesOnly)
        publishValues(std::move(projection->values));
    else
        publish(std::move(projection->snapshot), std::move(projection->changes), projection->reset);
}
} // namespace qcm::model
#include "Qcm/model/home_item_source.moc.cpp"
