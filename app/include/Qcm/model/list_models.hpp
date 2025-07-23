#pragma once

#include <memory_resource>
#include "kstore/qt/gadget_model.hpp"
#include "Qcm/model/store_item.hpp"

namespace qcm::model
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

    Q_PROPERTY(qcm::model::Album album READ album NOTIFY albumChanged)
    Q_PROPERTY(QQmlPropertyMap* extra READ extra NOTIFY albumChanged)
    Q_PROPERTY(qint32 discCount READ discCount NOTIFY discCountChanged)

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

class MixListModel
    : public kstore::QGadgetListModel,
      public kstore::QMetaListModelCRTP<model::Mix, MixListModel, kstore::ListStoreType::Map,
                                        std::pmr::polymorphic_allocator<model::Mix>> {
    Q_OBJECT
    QML_ANONYMOUS

    using list_crtp_t =
        kstore::QMetaListModelCRTP<model::Mix, MixListModel, kstore::ListStoreType::Map,
                                   std::pmr::polymorphic_allocator<model::Mix>>;
    using value_type = model::Mix;

public:
    MixListModel(QObject* parent = nullptr);
};

} // namespace qcm::model