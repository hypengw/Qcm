module;
#include "Qcm/model/home_item_source.moc.h"
#include "qml_material/model/item_source.hpp"
module qcm;
import :model.home_item_source;

namespace qcm::model
{
namespace
{
class HomeSnapshot final : public qml_material::TypedItemDataSnapshot<HomeCardData> {
public:
    HomeSnapshot(QList<HomeCardData> rows, QVector<quint64> versions)
        : m_rows(std::move(rows)), m_versions(std::move(versions)) {
        for (int i = 0; i < m_rows.size(); ++i) m_indexes.insert(m_rows[i].key, i);
    }
    int                          count() const override { return m_rows.size(); }
    qml_material::ItemDescriptor descriptorAt(int index) const override {
        return { m_rows.at(index).key, QStringLiteral("home-card"), m_versions.at(index) };
    }
    std::optional<int> indexOfKey(const QString& key) const override {
        auto found = m_indexes.constFind(key);
        return found == m_indexes.cend() ? std::nullopt : std::optional<int>(*found);
    }
    HomeCardData itemAt(int index) const override { return m_rows.at(index); }

private:
    const QList<HomeCardData> m_rows;
    const QVector<quint64>    m_versions;
    QHash<QString, int>       m_indexes;
};

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
    QList<HomeCardData> rows;
    const auto          append = [&]<class T>(T* model) {
        if (! model) return false;
        rows.reserve(model->rowCount());
        for (int i = 0; i < model->rowCount(); ++i) rows.append(cardData(model->at(i)));
        return true;
    };
    if (m_model && ! append(qobject_cast<ForYouListModel*>(m_model.data())) &&
        ! append(qobject_cast<RadioQueueListModel*>(m_model.data())) &&
        ! append(qobject_cast<AlbumListModel*>(m_model.data())) &&
        ! append(qobject_cast<MixListModel*>(m_model.data()))) {
        reject(QStringLiteral("Unsupported Home model"));
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
            old >= 0 && ! dirty && before->valueAt(old).value<HomeCardData>() == row;
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
    publish(std::make_shared<HomeSnapshot>(std::move(rows), std::move(versions)),
            reset ? QVector<qml_material::ItemChange>() : changes,
            reset);
}
} // namespace qcm::model
#include "Qcm/model/home_item_source.moc.cpp"
