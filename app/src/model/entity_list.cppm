export module qcm:model.entity_list;
export import :model.item_id_list;
export import :msg.store;
import qcm.core;
import qextra;
import rstd.cppstd;

export namespace qcm::model
{
template<class T>
class EntityList : public ItemIdListModel, public kstore::QListInterface {
public:
    using Id                         = std::remove_cvref_t<decltype(std::declval<T>().id_proto())>;
    using Ids                        = typename identity::IdTraits<Id>::List;
    static constexpr int ArtistsRole = ItemIdRole + 1;
    static constexpr int AlbumRole   = ItemIdRole + 2;
    static constexpr int StateRole   = ItemIdRole + 3;
    static constexpr int LoadedRole  = ItemIdRole + 4;
    static constexpr int StatusRole  = ItemIdRole + 5;
    EntityList(QObject* parent, enums::ItemType type, ReadStore<T> source);
    ~EntityList();
    void               set_store(QAbstractListModel*, ReadStore<T> source);
    const QMetaObject* rawItemMeta() const override;
    void*              rawAt(qint32 row) const override;
    void               rawAssign(qint32, const QVariant&) override {}
    QVariant           rawToVariant(void* data) const override;
    void               rawInsert(qint32, std::span<const QVariant>) override {}
    void               rawMove(qint32, qint32, qint32) override {}
    QVariant           rawKeyAt(qint32 row) const override;
    std::size_t        rawSize() const override;
    void               rawErase(qint32 first, qint32 last) override;
    QVariant           data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool               setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray>   roleNames() const override;
    i64                      key_at(qint32 row) const;
    T                        at(qint32 row) const;
    qsizetype                size() const;
    std::optional<qsizetype> query_idx(i64 id) const;
    void                     resetIds(const Ids& ids);
    void                     appendIds(const Ids& ids);
    void                     syncIds(const Ids& ids);

private:
    template<class Range>
    void resetKeys(const Range& ids);
    template<class Range>
    void syncKeys(const Range& ids);
    template<class Range>
    void appendKeys(const Range& ids);

    void                  pruneSelection();
    void                  reindex();
    enums::ItemType       m_type;
    ReadStore<T>          m_source;
    StoreSubscription     m_subscription;
    QList<i64>            m_ids;
    QHash<i64, qsizetype> m_rows;
    mutable QHash<i64, T> m_placeholders;
    bool                  m_bound {};
};

extern template class EntityList<Album>;
extern template class EntityList<Artist>;
extern template class EntityList<Mix>;
extern template class EntityList<Song>;
} // namespace qcm::model
