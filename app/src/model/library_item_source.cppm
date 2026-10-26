module;
#include "qml_material/model/item_source.hpp"
#include "qml_material/model/item_demand.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/model/library_item_source.moc"
#endif
export module qcm:model.library_item_source;
export import :model.list_models;

export namespace qcm::model
{
class LibraryCardData {
    Q_GADGET
    QML_VALUE_TYPE(library_card_data)
    Q_PROPERTY(QString key MEMBER key)
    Q_PROPERTY(qcm::model::ItemId itemId MEMBER itemId)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QList<qcm::model::Artist> artists MEMBER artists)
    Q_PROPERTY(quint32 trackCount MEMBER trackCount)
    Q_PROPERTY(quint32 albumCount MEMBER albumCount)
    Q_PROPERTY(quint32 musicCount MEMBER musicCount)
public:
    QString       key, name;
    ItemId        itemId;
    QList<Artist> artists;
    quint32       trackCount {}, albumCount {}, musicCount {};
    bool          operator==(const LibraryCardData&) const = default;
};
class LibraryItemSource : public qml_material::ItemSource {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged FINAL)
public:
    using ItemSource::ItemSource;
    QAbstractItemModel* model() const { return m_model; }
    void                setModel(QAbstractItemModel*);
    Q_INVOKABLE void    requestMore(const qml_material::ItemDemand&);
    Q_SIGNAL void       modelChanged();

private:
    void                           rebuild(int first = -1, int last = -1);
    QPointer<QAbstractItemModel>   m_model;
    QList<QMetaObject::Connection> m_connections;
    qml_material::ItemDemand       m_demand;
    std::optional<int>             m_requestedCount;
    quint64                        m_generation  = 0;
    bool                           m_fetchQueued = false;
};
} // namespace qcm::model
