module;
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlPropertyMap>
#include "Qcm/model/list_models.moc.h"
#ifdef Q_MOC_RUN
#include "Qcm/model/list_models.moc"
#endif
#include "kstore/qt/gadget_model.hpp"

export module qcm.model.list_models;
export import qcm.model.store_item;
export import qcm.global;

export namespace qcm::model
{

template<typename TItem, typename CRTP>
using MetaListCRTP = kstore::QMetaListModelCRTP<TItem, CRTP, kstore::ListStoreType::Share,
                                                std::pmr::polymorphic_allocator<TItem>>;

class AlbumListModel : public kstore::QGadgetListModel,
                       public MetaListCRTP<model::Album, AlbumListModel> {
    Q_OBJECT
    QML_ANONYMOUS

    using list_crtp_t = MetaListCRTP<model::Album, AlbumListModel>;
    using value_type  = model::Album;

public:
    AlbumListModel(QObject* parent = nullptr);

    Q_INVOKABLE QQmlPropertyMap* extra(i32 idx) const;
};

class SongListModel : public kstore::QGadgetListModel,
                      public MetaListCRTP<model::Song, SongListModel> {
    Q_OBJECT
    QML_ANONYMOUS
    using list_crtp_t = MetaListCRTP<model::Song, SongListModel>;
    using value_type  = model::Song;

public:
    SongListModel(QObject* parent = nullptr);

    Q_INVOKABLE QQmlPropertyMap* extra(i32 idx) const;
};

class AlbumSongListModel : public kstore::QGadgetListModel,
                           public MetaListCRTP<model::Song, AlbumSongListModel> {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(qcm::model::Album album READ album NOTIFY albumChanged FINAL)
    Q_PROPERTY(QQmlPropertyMap* extra READ extra NOTIFY albumChanged FINAL)
    Q_PROPERTY(qint32 discCount READ discCount NOTIFY discCountChanged FINAL)

    using list_crtp_t = MetaListCRTP<model::Song, AlbumSongListModel>;
    using album_type  = model::Album;
    using value_type  = model::Song;

public:
    AlbumSongListModel(QObject* parent = nullptr);
    ~AlbumSongListModel();

    auto album() const -> album_type;
    auto extra() const -> QQmlPropertyMap*;
    auto discCount() const -> qint32;

    void setAlbum(const album_type&);
    void setDiscCount(qint32);

    Q_SIGNAL void albumChanged();
    Q_SIGNAL void discCountChanged();

private:
    model::AlbumStoreItem m_item;
    qint32                m_disc_count;
};

class ArtistListModel : public kstore::QGadgetListModel,
                        public MetaListCRTP<model::Artist, ArtistListModel> {
    Q_OBJECT
    QML_ANONYMOUS

    using list_crtp_t = MetaListCRTP<model::Artist, ArtistListModel>;
    using value_type  = model::Artist;

public:
    ArtistListModel(QObject* parent = nullptr);
};

class MixListModel : public kstore::QGadgetListModel,
                     public MetaListCRTP<model::Mix, MixListModel> {
    Q_OBJECT
    QML_ANONYMOUS

    using list_crtp_t = MetaListCRTP<model::Mix, MixListModel>;
    using value_type  = model::Mix;

public:
    MixListModel(QObject* parent = nullptr);
};

class MixSongListModel : public kstore::QGadgetListModel,
                         public MetaListCRTP<model::Song, MixSongListModel> {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(qcm::model::Mix mix READ mix NOTIFY mixChanged FINAL)
    Q_PROPERTY(QQmlPropertyMap* extra READ extra NOTIFY mixChanged FINAL)
    Q_PROPERTY(qint32 discCount READ discCount NOTIFY discCountChanged FINAL)

    using list_crtp_t = MetaListCRTP<model::Song, MixSongListModel>;
    using mix_type    = model::Mix;
    using value_type  = model::Song;

public:
    MixSongListModel(QObject* parent = nullptr);
    ~MixSongListModel();

    auto mix() const -> mix_type;
    auto extra() const -> QQmlPropertyMap*;
    auto discCount() const -> qint32;

    void setMix(const mix_type&);
    void setDiscCount(qint32);

    Q_SIGNAL void mixChanged();
    Q_SIGNAL void discCountChanged();

private:
    model::MixStoreItem m_item;
    qint32              m_disc_count;
};

} // namespace qcm::model

module :private;

namespace qcm::model
{
AlbumListModel::AlbumListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}
QQmlPropertyMap* AlbumListModel::extra(i32 idx) const {
    if (auto extend = AppStore::instance()->albums.query_extend(this->key_at(idx)); extend) {
        return extend->extra.get();
    }
    return nullptr;
}
SongListModel::SongListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}
QQmlPropertyMap* SongListModel::extra(i32 idx) const {
    if (auto extend = AppStore::instance()->songs.query_extend(this->key_at(idx)); extend) {
        return extend->extra.get();
    }
    return nullptr;
}

AlbumSongListModel::AlbumSongListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }),
      m_disc_count(1) {
    connect(&m_item, &model::AlbumStoreItem::itemChanged, this, &AlbumSongListModel::albumChanged);
}
AlbumSongListModel::~AlbumSongListModel() {}
auto AlbumSongListModel::album() const -> album_type { return m_item.item(); }
auto AlbumSongListModel::discCount() const -> qint32 { return m_disc_count; }
void AlbumSongListModel::setAlbum(const album_type& album) { m_item.setItem(album); }
void AlbumSongListModel::setDiscCount(qint32 c) {
    if (c != m_disc_count) {
        m_disc_count = c;
        discCountChanged();
    }
}
auto AlbumSongListModel::extra() const -> QQmlPropertyMap* { return m_item.extra(); }

ArtistListModel::ArtistListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}

MixListModel::MixListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }) {}

MixSongListModel::MixSongListModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent),
      list_crtp_t(list_crtp_t::allocator_type { mem_mgr().store_mem }),
      m_disc_count(1) {
    connect(&m_item, &model::MixStoreItem::itemChanged, this, &MixSongListModel::mixChanged);
}
MixSongListModel::~MixSongListModel() {}
auto MixSongListModel::mix() const -> mix_type { return m_item.item(); }
auto MixSongListModel::discCount() const -> qint32 { return m_disc_count; }
void MixSongListModel::setMix(const mix_type& mix) { m_item.setItem(mix); }
void MixSongListModel::setDiscCount(qint32 c) {
    if (c != m_disc_count) {
        m_disc_count = c;
        discCountChanged();
    }
}
auto MixSongListModel::extra() const -> QQmlPropertyMap* { return m_item.extra(); }

} // namespace qcm::model

#include "Qcm/model/list_models.moc.cpp"