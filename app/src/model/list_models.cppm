module;
#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#include "Qcm/model/list_models.moc"
#endif

export module qcm:model.list_models;
export import :model.store_item;
export import :model.item_id_list;
export import :model.entity_list;
export import :global;

export namespace qcm::model
{

class AlbumListModel : public EntityList<model::Album> {
    Q_OBJECT
    QML_ANONYMOUS

    using value_type  = model::Album;

public:
    AlbumListModel(QObject* parent = nullptr);

};

class SongListModel : public EntityList<model::Song> {
    Q_OBJECT
    QML_ANONYMOUS
    using value_type  = model::Song;

public:
    SongListModel(QObject* parent = nullptr);

};

class AlbumSongListModel : public EntityList<model::Song> {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(qcm::model::Album album READ album NOTIFY albumChanged FINAL)
    Q_PROPERTY(qint32 discCount READ discCount NOTIFY discCountChanged FINAL)

    using album_type  = model::Album;
    using value_type  = model::Song;

public:
    AlbumSongListModel(QObject* parent = nullptr);
    ~AlbumSongListModel();

    auto album() const -> album_type;
    auto discCount() const -> qint32;

    void setAlbumId(i64);
    void setDiscCount(qint32);

    Q_SIGNAL void albumChanged();
    Q_SIGNAL void discCountChanged();

private:
    model::AlbumStoreItem m_item;
    qint32                m_disc_count;
};

class ArtistListModel : public EntityList<model::Artist> {
    Q_OBJECT
    QML_ANONYMOUS

    using value_type  = model::Artist;

public:
    ArtistListModel(QObject* parent = nullptr);
};

class MixListModel : public EntityList<model::Mix> {
    Q_OBJECT
    QML_ANONYMOUS

    using value_type  = model::Mix;

public:
    MixListModel(QObject* parent = nullptr);
};

class ForYouListModel : public kstore::QGadgetListModel,
    public kstore::QMetaListModelCRTP<model::ForYouCard, ForYouListModel, kstore::ListStoreType::VectorWithMap> {
    Q_OBJECT
    QML_ANONYMOUS
public:
    ForYouListModel(QObject* parent = nullptr);
    auto data(const QModelIndex&, int role = 0) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
};

class RadioQueueListModel : public ItemIdListModel,
                            public kstore::QMetaListModelCRTP<model::RadioQueue, RadioQueueListModel, kstore::ListStoreType::Vector> {
    Q_OBJECT
    QML_ANONYMOUS

    using list_crtp_t = kstore::QMetaListModelCRTP<model::RadioQueue, RadioQueueListModel, kstore::ListStoreType::Vector>;
    using value_type  = model::RadioQueue;

public:
    static constexpr int RadioRole = ItemIdRole + 1;
    RadioQueueListModel(QObject* parent = nullptr);
    auto data(const QModelIndex&, int role = 0) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
};

class MixSongListModel : public EntityList<model::Song> {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(qcm::model::Mix mix READ mix NOTIFY mixChanged FINAL)
    Q_PROPERTY(qint32 discCount READ discCount NOTIFY discCountChanged FINAL)

    using mix_type    = model::Mix;
    using value_type  = model::Song;

public:
    MixSongListModel(QObject* parent = nullptr);
    ~MixSongListModel();

    auto mix() const -> mix_type;
    auto discCount() const -> qint32;

    void setMixId(i64);
    void setDiscCount(qint32);

    Q_SIGNAL void mixChanged();
    Q_SIGNAL void discCountChanged();

private:
    model::MixStoreItem m_item;
    qint32              m_disc_count;
};

} // namespace qcm::model
