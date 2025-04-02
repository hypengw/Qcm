#pragma once

#include <QQmlEngine>

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/backend_msg.hpp"
#include "Qcm/query/query.hpp"

namespace qcm
{
class AlbumListModel
    : public meta_model::QGadgetListModel<msg::model::Album, meta_model::QMetaListStore::Share> {
    Q_OBJECT
    using base_type =
        meta_model::QGadgetListModel<msg::model::Album, meta_model::QMetaListStore::Share>;

    using value_type = msg::model::Album;

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
    : public meta_model::QGadgetListModel<msg::model::Song, meta_model::QMetaListStore::Share> {
    Q_OBJECT

    Q_PROPERTY(qcm::msg::model::Album album READ album NOTIFY albumChanged)
    using base_type =
        meta_model::QGadgetListModel<msg::model::Song, meta_model::QMetaListStore::Share>;

    using album_type  = msg::model::Album;
    using value_type = msg::model::Song;

public:
    AlbumSongListModel(QObject* parent = nullptr);

    auto album() -> const album_type&;
    void setAlbum(const album_type&);

    Q_SIGNAL void albumChanged();

private:
    album_type m_item;
};

class AlbumQuery : public query::QueryList<AlbumSongListModel> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    AlbumQuery(QObject* parent = nullptr);
    void reload() override;

    auto itemId() const -> QString;
    void setItemId(QStringView);

    Q_SIGNAL void itemIdChanged();

private:
    QString m_item_id;
};

} // namespace qcm
