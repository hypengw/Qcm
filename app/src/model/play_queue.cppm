module;
#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/model/play_queue.moc"
#endif

export module qcm:queue;
export import :model.id_queue;
import :model.item_id_list;
export import :msg;
import :backend;
import qextra;

export namespace qcm
{
class PlayQueue : public QAbstractListModel, public kstore::QMetaRoleNames {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qint32 currentIndex READ currentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(QString currentEntryId READ currentEntryId NOTIFY currentEntryChanged FINAL)
    Q_PROPERTY(qcm::model::Song currentSong READ currentSong NOTIFY currentSongChanged FINAL)
    Q_PROPERTY(bool changingTrack READ changingTrack NOTIFY changingTrackChanged FINAL)
    Q_PROPERTY(qcm::model::ItemId currentSongId READ currentSongId NOTIFY currentSongChanged FINAL)
    Q_PROPERTY(
        qcm::model::ItemId currentAlbumId READ currentAlbumId NOTIFY currentSongChanged FINAL)
    Q_PROPERTY(
        qcm::enums::LoopMode loopMode READ loopMode WRITE setLoopMode NOTIFY loopModeChanged FINAL)
    Q_PROPERTY(bool randomMode READ randomMode WRITE setRandomMode NOTIFY randomModeChanged FINAL)
    Q_PROPERTY(bool canNext READ canNext NOTIFY canNextChanged FINAL)
    Q_PROPERTY(bool canPrev READ canPrev NOTIFY canPrevChanged FINAL)
    Q_PROPERTY(bool canJump READ canJump NOTIFY canJumpChanged FINAL)
    Q_PROPERTY(bool canRemove READ canRemove NOTIFY canRemoveChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged FINAL)
    Q_PROPERTY(
        qcm::model::DynamicIdQueue* radioQueue READ radioQueue NOTIFY radioQueueChanged FINAL)

public:
    using LoopMode = enums::LoopMode;
    using Song     = model::Song;
    using SongItem = AppStore::song_item;

    explicit PlayQueue(QObject* parent = nullptr);
    ~PlayQueue();
    void bindBackend(Backend*);
    void applyState(const msg::PlayerQueueMsg&);
    void drop_global();
    void reloadEntities();
    void sendRequest(msg::PlayerQueueReq, std::function<void(bool)> completed = {});
    void playSong(model::ItemId, std::function<void(bool)> completed = {});
    void replaceSongs(std::span<const model::ItemId>, std::function<void(bool)> completed = {});
    void appendSongs(std::span<const model::ItemId>);
    void insertNext(std::span<const model::ItemId>);

    auto rowCount(const QModelIndex& parent = {}) const -> int override;
    auto data(const QModelIndex&, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
    bool removeRows(int row, int count, const QModelIndex& parent = {}) override;
    bool moveRows(const QModelIndex&, int row, int count, const QModelIndex&,
                  int destination) override;

    auto        currentId() const -> rstd::Option<model::ItemId>;
    auto        getId(qint32) const -> rstd::Option<model::ItemId>;
    auto        currentIndex() const -> qint32;
    auto        currentEntryId() const -> QString;
    auto        currentSong() const -> Song;
    auto        currentSongId() const -> model::ItemId;
    auto        currentAlbumId() const -> model::ItemId;
    auto        changingTrack() const -> bool;
    auto        loopMode() const -> LoopMode;
    void        setLoopMode(LoopMode);
    auto        randomMode() const -> bool;
    void        setRandomMode(bool);
    auto        canNext() const -> bool;
    auto        canPrev() const -> bool;
    auto        canJump() const -> bool;
    auto        canRemove() const -> bool;
    auto        name() const -> QString;
    auto        radioQueue() const -> model::DynamicIdQueue*;
    Q_INVOKABLE model::DynamicIdQueue* dynamicQueue(const msg::id::RadioQueueId&, const QString&,
                                                    const QList<msg::model::QueueConfigField>&,
                                                    bool can_restart, const QString& restart_label);

    Q_SLOT void      next();
    Q_SLOT void      prev();
    Q_SLOT void      iterLoopMode();
    Q_SLOT void      clear();
    Q_INVOKABLE void select(qint32 index);
    Q_INVOKABLE bool move(qint32 source, qint32 destination, qint32 count = 1);

    Q_SIGNAL void currentIndexChanged(qint32);
    Q_SIGNAL void currentEntryChanged();
    Q_SIGNAL void currentSongChanged();
    Q_SIGNAL void changingTrackChanged();
    Q_SIGNAL void loopModeChanged(LoopMode);
    Q_SIGNAL void randomModeChanged(bool);
    Q_SIGNAL void canNextChanged();
    Q_SIGNAL void canPrevChanged();
    Q_SIGNAL void canJumpChanged();
    Q_SIGNAL void canRemoveChanged();
    Q_SIGNAL void nameChanged();
    Q_SIGNAL void radioQueueChanged();

private:
    void disconnectQueue();
    void notifyRowsForIds(const std::unordered_set<model::ItemId>&);
    void fetchSongs();
    void sendSongs(msg::PlayerQueueActionGadget::PlayerQueueAction, std::span<const model::ItemId>,
                   bool play, std::function<void(bool)> completed = {});

    QPointer<Backend>                                    m_backend;
    msg::PlayerQueueMsg                                  m_state;
    msg::PlayerStateMsg                                  m_player_state;
    bool                                                 m_has_snapshot {};
    bool                                                 m_has_player_snapshot {};
    bool                                                 m_available {};
    bool                                                 m_mutating {};
    StoreSubscription                                    m_subscription;
    std::unordered_map<model::ItemId, SongItem>          m_songs;
    std::unordered_set<model::ItemId>                    m_pending_ids;
    std::map<quint64, std::unordered_set<model::ItemId>> m_loading_ids;
    std::map<quint64, StoreSubscription>                 m_loads;
    quint64                                              m_load_serial {};
    quint64                                              m_entity_generation {};
    QMap<qint64, model::DynamicIdQueue*>                 m_radios;
};
} // namespace qcm
