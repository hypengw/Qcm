module;
#include "Qcm/message/entity_store.hpp"
#include "kstore/qt/meta_list_model.hpp"
export module qcm:model.entity_list;
export import :model.item_id_list;
export import :msg.store;
import qextra;

export namespace qcm::model
{
template<class T>
class EntityList : public ItemIdListModel, public kstore::QListInterface {
public:
    static constexpr int ArtistsRole = ItemIdRole + 1;
    static constexpr int AlbumRole   = ItemIdRole + 2;
    static constexpr int StateRole   = ItemIdRole + 3;
    static constexpr int LoadedRole  = ItemIdRole + 4;
    static constexpr int StatusRole  = ItemIdRole + 5;
    EntityList(QObject* parent, enums::ItemType type, ReadStore<T> source)
        : ItemIdListModel(this, parent, type), m_type(type) {
        set_store(this, std::move(source));
    }
    ~EntityList() {
        m_subscription.reset();
        for (auto id : m_ids) m_source.unpin(id);
    }
    void set_store(QAbstractListModel*, ReadStore<T> source) {
        if (m_bound && m_source == source) return;
        if (m_bound) resetIds(QList<i64>());
        m_subscription.reset();
        m_source       = std::move(source);
        m_bound        = true;
        m_subscription = m_source.subscribe([this](i64 id, quint64 fields) {
            auto row = m_rows.value(id, -1);
            if (row < 0) return;
            QList<int> roles;
            const auto names = roleNames();
            for (auto it = names.cbegin(); it != names.cend(); ++it) {
                auto slot = store::field(it.value());
                if (slot && (fields & store::bit(*slot))) roles.push_back(it.key());
            }
            if (fields & store::bit(store::Field::Artists)) roles.push_back(ArtistsRole);
            if (fields & store::bit(store::Field::Album)) roles.push_back(AlbumRole);
            if (fields & store::bit(store::Field::Favorite)) roles.push_back(StateRole);
            if (fields & (store::bit(store::Field::Presence) | store::bit(store::Field::Deleted))) {
                roles.push_back(LoadedRole);
                for (auto role : names.keys())
                    if (! roles.contains(role)) roles.push_back(role);
            }
            if (fields & (store::bit(store::Field::Presence) | store::bit(store::Field::Stale) |
                          store::bit(store::Field::Deleted)))
                roles.push_back(StatusRole);
            if (! roles.isEmpty()) dataChanged(index(row, 0), index(row, 0), roles);
        });
    }
    const QMetaObject* rawItemMeta() const override { return &T::staticMetaObject; }
    void*              rawAt(qint32 row) const override {
        auto id = m_ids.at(row);
        if (auto value = m_source.store_query(id)) return const_cast<T*>(value);
        auto& placeholder = m_placeholders[id];
        placeholder.setId_proto(id);
        return &placeholder;
    }
    void     rawAssign(qint32, const QVariant&) override {}
    QVariant rawToVariant(void* data) const override {
        return QVariant::fromValue(*static_cast<const T*>(data));
    }
    void     rawInsert(qint32, std::span<const QVariant>) override {}
    void     rawMove(qint32, qint32, qint32) override {}
    QVariant rawKeyAt(qint32 row) const override {
        return row >= 0 && row < m_ids.size() ? QVariant(qint64(m_ids[row])) : QVariant();
    }
    std::size_t rawSize() const override { return m_ids.size(); }
    void        rawErase(qint32 first, qint32 last) override {
        for (auto i = first; i < last; ++i) m_source.unpin(m_ids[i]);
        m_ids.remove(first, last - first);
        reindex();
    }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (! index.isValid() || index.row() < 0 || index.row() >= m_ids.size()) return {};
        ItemId id(m_type, m_ids[index.row()]);
        if (role == ArtistsRole) return QVariant::fromValue(AppStore::instance()->artistsFor(id));
        if (role == AlbumRole) return QVariant::fromValue(AppStore::instance()->albumFor(id));
        if (role == StateRole) return QVariant::fromValue(AppStore::instance()->stateFor(id));
        if (role == LoadedRole) return m_source.store_query(id.id()) != nullptr;
        if (role == StatusRole) return AppStore::instance()->status(id);
        return ItemIdListModel::data(index, role);
    }
    bool setData(const QModelIndex& index, const QVariant& value, int role) override {
        return role == kstore::QMetaListModel::SelectedRole &&
               ItemIdListModel::setData(index, value, role);
    }
    QHash<int, QByteArray> roleNames() const override {
        auto names = ItemIdListModel::roleNames();
        names.insert(ArtistsRole, "artists");
        names.insert(AlbumRole, "album");
        names.insert(StateRole, "state");
        names.insert(LoadedRole, "loaded");
        names.insert(StatusRole, "entityStatus");
        return names;
    }
    i64                      key_at(qint32 row) const { return m_ids.at(row); }
    T                        at(qint32 row) const { return *static_cast<const T*>(rawAt(row)); }
    qsizetype                size() const { return m_ids.size(); }
    std::optional<qsizetype> query_idx(i64 id) const {
        auto it = m_rows.constFind(id);
        return it == m_rows.cend() ? std::nullopt : std::optional<qsizetype>(it.value());
    }
    template<class Range>
    void resetIds(const Range& ids) {
        beginResetModel();
        for (auto id : m_ids) m_source.unpin(id);
        m_ids.clear();
        m_rows.clear();
        m_placeholders.clear();
        for (auto id : ids)
            if (id > 0 && ! m_rows.contains(id)) {
                m_rows.insert(id, m_ids.size());
                m_ids.push_back(id);
                m_source.pin(id);
            }
        endResetModel();
        pruneSelection();
    }
    template<class Range>
    void syncIds(const Range& ids) {
        QList<i64> wanted;
        QSet<i64>  set;
        for (auto id : ids)
            if (id > 0 && ! set.contains(id)) {
                wanted.push_back(id);
                set.insert(id);
            }
        for (qsizetype i = m_ids.size(); i-- > 0;)
            if (! set.contains(m_ids[i])) {
                beginRemoveRows({}, i, i);
                m_source.unpin(m_ids[i]);
                m_ids.removeAt(i);
                reindex();
                endRemoveRows();
            }
        for (qsizetype i = 0; i < wanted.size(); ++i) {
            if (i < m_ids.size() && m_ids[i] == wanted[i]) continue;
            auto existing = m_rows.value(wanted[i], -1);
            if (existing >= 0) {
                beginMoveRows({}, existing, existing, {}, i);
                m_ids.move(existing, i);
                reindex();
                endMoveRows();
            } else {
                beginInsertRows({}, i, i);
                m_ids.insert(i, wanted[i]);
                m_source.pin(wanted[i]);
                reindex();
                endInsertRows();
            }
        }
        pruneSelection();
    }
    template<class Range>
    void appendIds(const Range& ids) {
        auto combined = m_ids;
        for (auto id : ids) combined.push_back(id);
        syncIds(combined);
    }

private:
    void pruneSelection() {
        QStringList keys;
        for (auto id : m_ids) {
            auto key = QString::number(id);
            if (m_selected_keys.contains(key)) keys.push_back(key);
        }
        setSelectedKeys(keys);
    }
    void reindex() {
        m_rows.clear();
        for (qsizetype i = 0; i < m_ids.size(); ++i) m_rows.insert(m_ids[i], i);
    }
    enums::ItemType       m_type;
    ReadStore<T>          m_source;
    StoreSubscription     m_subscription;
    QList<i64>            m_ids;
    QHash<i64, qsizetype> m_rows;
    mutable QHash<i64, T> m_placeholders;
    bool                  m_bound {};
};
} // namespace qcm::model
