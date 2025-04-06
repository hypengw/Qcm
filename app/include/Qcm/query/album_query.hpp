#pragma once

#include <QQmlEngine>

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/query/query.hpp"
#include "Qcm/model/store_item.hpp"

namespace qcm
{
class AlbumListModel
    : public meta_model::QGadgetListModel<model::Album, meta_model::QMetaListStore::Share> {
    Q_OBJECT
    using base_type = meta_model::QGadgetListModel<model::Album, meta_model::QMetaListStore::Share>;

    using value_type = model::Album;

public:
    AlbumListModel(QObject* parent = nullptr);

    Q_INVOKABLE QQmlPropertyMap* extra(i32 idx) const;
};

class AlbumsQuery : public query::QueryList<AlbumListModel> {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumsQuery(QObject* parent = nullptr);
    void reload() override;
    void fetchMore(qint32) override;
};

class AlbumSongListModel
    : public meta_model::QGadgetListModel<model::Song, meta_model::QMetaListStore::Share> {
    Q_OBJECT

    Q_PROPERTY(qcm::model::Album album READ album NOTIFY albumChanged)
    Q_PROPERTY(QQmlPropertyMap* extra READ extra NOTIFY albumChanged)
    using base_type = meta_model::QGadgetListModel<model::Song, meta_model::QMetaListStore::Share>;

    using album_type = model::Album;
    using value_type = model::Song;

public:
    AlbumSongListModel(QObject* parent = nullptr);
    ~AlbumSongListModel();

    auto album() const -> album_type;
    auto extra() const -> QQmlPropertyMap*;

    void setAlbum(const album_type&);

    Q_SIGNAL void albumChanged();

private:
    model::AlbumStoreItem m_item;
};

class AlbumQuery : public query::QueryList<AlbumSongListModel> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    AlbumQuery(QObject* parent = nullptr);
    void reload() override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

} // namespace qcm
