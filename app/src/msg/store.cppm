module;
#ifdef Q_MOC_RUN
#    include "QExtra/macro_qt.hpp"
#    include "Qcm/msg/store.moc"
#else
#    include "Qcm/message/entity_store.hpp"
#endif
export module qcm:msg.store;
export import :msg.backend;
export import :model.share_store;
import qextra;

export namespace qcm
{
class EntityView;
class AppStore : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Store)
    QML_SINGLETON
public:
    explicit AppStore(QObject* parent);
    ~AppStore();
    AppStore() = delete;
    static AppStore*        instance();
    static AppStore*        create(QQmlEngine*, QJSEngine*);
    Q_INVOKABLE EntityView* item(model::ItemId);
    void                    deliver(msg::entity::EntityBatch, std::function<void(QString)>);
    void                    reset();
    void                    markStale();
    quint64                 scopeRevision() const { return m_data.scopeRevision(); }
    int                     status(model::ItemId) const;
    void                    collect();
    qsizetype               size() const { return m_data.size(); }
    qsizetype               bytes() const { return m_data.bytes(); }
    void                    pin(model::ItemId);
    void                    unpin(model::ItemId);
    const store::Entry*     find(model::ItemId) const;
    QList<model::Artist>    artistsFor(model::ItemId) const;
    model::Album            albumFor(model::ItemId) const;
    msg::entity::ItemState  stateFor(model::ItemId) const;
    StoreSubscription       subscribe(std::function<void(model::ItemId, quint64)>);
    StoreSubscription       subscribe(model::ItemId, std::function<void(quint64)>);
    Q_SIGNAL void           changed(model::ItemId, quint64 fields);
    Q_SIGNAL void           stateChanged(model::ItemId);
    Q_SIGNAL void           scopeChanged();
    using album_store  = ReadStore<model::Album>;
    using song_store   = ReadStore<model::Song>;
    using artist_store = ReadStore<model::Artist>;
    using mix_store    = ReadStore<model::Mix>;
    using album_item   = album_store::Handle;
    using song_item    = song_store::Handle;
    album_store  albums;
    song_store   songs;
    artist_store artists;
    mix_store    mixes;

private:
    template<class T>
    ReadStore<T> reader(int type) {
        auto weak = QPointer<AppStore>(this);
        return ReadStore<T>(
            this,
            [weak, type](i64 id) -> const T* {
                if (! weak) return nullptr;
                auto entry = weak->m_data.find({ type, id });
                return entry ? static_cast<const T*>(entry->data.constData()) : nullptr;
            },
            [weak, type](auto callback) -> StoreSubscription {
                if (! weak) return {};
                return weak->subscribe(
                    [type, callback = std::move(callback)](model::ItemId id, quint64 fields) {
                        auto actual = int(id.type());
                        if (actual == 52) actual = 53;
                        if (actual == type) callback(id.id(), fields);
                    });
            },
            [weak, type](i64 id) {
                if (weak) weak->m_data.pin({ type, id });
            },
            [weak, type](i64 id) {
                if (weak) weak->unpin(model::ItemId(static_cast<enums::ItemType>(type), id));
            });
    }
    void                                       publish(const QList<store::Change>&);
    store::EntityStore                         m_data;
    QHash<model::ItemId, QPointer<EntityView>> m_views;
    QHash<store::Key, QMap<quint64, std::function<void(quint64)>>> m_observers;
    quint64                                                        m_observerSerial {};
    void                                                           enqueue(std::function<void()>);
    QList<std::function<void()>>                                   m_pending;
    quint64                                                        m_deliveryGeneration {};
    bool                                                           m_publishing {};
    bool                                                           m_collectScheduled {};
};

class EntityView : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY changed FINAL)
    Q_PROPERTY(QString name READ name NOTIFY changed FINAL)
    Q_PROPERTY(QList<qcm::model::Artist> artists READ artists NOTIFY changed FINAL)
    Q_PROPERTY(qcm::model::ItemId albumId READ albumId NOTIFY changed FINAL)
    Q_PROPERTY(qcm::model::Album album READ album NOTIFY changed FINAL)
    Q_PROPERTY(qcm::msg::entity::ItemState state READ state NOTIFY changed FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY changed FINAL)
    Q_PROPERTY(Status status READ status NOTIFY changed FINAL)
    Q_PROPERTY(bool artistsKnown READ artistsKnown NOTIFY changed FINAL)
    Q_PROPERTY(bool albumKnown READ albumKnown NOTIFY changed FINAL)
    Q_PROPERTY(bool stateKnown READ stateKnown NOTIFY changed FINAL)
    Q_PROPERTY(quint64 knownFields READ knownFields NOTIFY changed FINAL)
    Q_PROPERTY(quint64 nullFields READ nullFields NOTIFY changed FINAL)
public:
    enum Status
    {
        Missing,
        Ready,
        Stale,
        Deleted,
        Invalidated,
        Evicted
    };
    Q_ENUM(Status)
    Status status() const;
    explicit EntityView(QObject* parent = nullptr);
    ~EntityView();
    model::ItemId          itemId() const;
    QString                name() const;
    void                   setItemId(model::ItemId);
    QList<model::Artist>   artists() const;
    model::ItemId         albumId() const;
    model::Album           album() const;
    msg::entity::ItemState state() const;
    bool                   loaded() const;
    bool                   artistsKnown() const;
    bool                   albumKnown() const;
    bool                   stateKnown() const;
    quint64                knownFields() const;
    quint64                nullFields() const;
    Q_SIGNAL void          changed();

protected:
    AppStore* store() const { return m_store; }

private:
    QPointer<AppStore> m_store;
    model::ItemId      m_id;
    StoreSubscription  m_subscription;
};
} // namespace qcm
