#pragma once

#include <QQmlEngine>

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/query/query.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{
class ArtistsQuery : public QueryList, public QueryExtra<model::ArtistListModel, ArtistsQuery> {
    Q_OBJECT
    QML_ELEMENT
public:
    ArtistsQuery(QObject* parent = nullptr);
    void reload() override;
    void fetchMore(qint32) override;
};

class AlbumArtistsQuery : public QueryList, public QueryExtra<model::ArtistListModel, AlbumArtistsQuery> {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumArtistsQuery(QObject* parent = nullptr);
    void reload() override;
    void fetchMore(qint32) override;
};

class ArtistQuery : public Query, public QueryExtra<model::ArtistStoreItem, ArtistQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    ArtistQuery(QObject* parent = nullptr);
    void reload() override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

class ArtistAlbumQuery : public QueryList, public QueryExtra<model::AlbumListModel, ArtistAlbumQuery> {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    ArtistAlbumQuery(QObject* parent = nullptr);
    void reload() override;
    void fetchMore(qint32) override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

} // namespace qcm
