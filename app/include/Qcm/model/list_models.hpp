#pragma once

#include <memory_resource>
#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/model/store_item.hpp"

namespace qcm::model
{
class AlbumListModel
    : public meta_model::QGadgetListModel<model::Album, meta_model::QMetaListStore::Share,
                                          std::pmr::polymorphic_allocator<model::Album>> {
    Q_OBJECT
    QML_ANONYMOUS
    using base_type = meta_model::QGadgetListModel<model::Album, meta_model::QMetaListStore::Share,
                                                   std::pmr::polymorphic_allocator<model::Album>>;

    using value_type = model::Album;

public:
    AlbumListModel(QObject* parent = nullptr);

    Q_INVOKABLE QQmlPropertyMap* extra(i32 idx) const;
};

class SongListModel
    : public meta_model::QGadgetListModel<model::Song, meta_model::QMetaListStore::Share,
                                          std::pmr::polymorphic_allocator<model::Song>> {
    Q_OBJECT
    QML_ANONYMOUS
    using base_type = meta_model::QGadgetListModel<model::Song, meta_model::QMetaListStore::Share,
                                                   std::pmr::polymorphic_allocator<model::Song>>;

    using value_type = model::Song;

public:
    SongListModel(QObject* parent = nullptr);

    Q_INVOKABLE QQmlPropertyMap* extra(i32 idx) const;
};

class AlbumSongListModel
    : public meta_model::QGadgetListModel<model::Song, meta_model::QMetaListStore::Share,
                                          std::pmr::polymorphic_allocator<model::Song>> {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(qcm::model::Album album READ album NOTIFY albumChanged)
    Q_PROPERTY(QQmlPropertyMap* extra READ extra NOTIFY albumChanged)
    Q_PROPERTY(qint32 discCount READ discCount NOTIFY discCountChanged)
    using base_type = meta_model::QGadgetListModel<model::Song, meta_model::QMetaListStore::Share,
                                                   std::pmr::polymorphic_allocator<model::Song>>;

    using album_type = model::Album;
    using value_type = model::Song;

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
    qint32 m_disc_count;
};

class ArtistListModel
    : public meta_model::QGadgetListModel<model::Artist, meta_model::QMetaListStore::Share,
                                          std::pmr::polymorphic_allocator<model::Artist>> {
    Q_OBJECT
    QML_ANONYMOUS
    using base_type = meta_model::QGadgetListModel<model::Artist, meta_model::QMetaListStore::Share,
                                                   std::pmr::polymorphic_allocator<model::Artist>>;

    using value_type = model::Artist;

public:
    ArtistListModel(QObject* parent = nullptr);
};

class MixListModel
    : public meta_model::QGadgetListModel<model::Mix, meta_model::QMetaListStore::VectorWithMap> {
    Q_OBJECT
    QML_ANONYMOUS
    using base_type =
        meta_model::QGadgetListModel<model::Mix, meta_model::QMetaListStore::VectorWithMap>;

    using value_type = model::Mix;

public:
    MixListModel(QObject* parent = nullptr);
};

} // namespace qcm::model