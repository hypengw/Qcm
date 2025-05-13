#pragma once

#include <QQmlEngine>
#include "Qcm/query/query.hpp"
#include "Qcm/model/store_item.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{
class AlbumsQuery : public QueryList, public QueryExtra<model::AlbumListModel, AlbumsQuery> {
    Q_OBJECT
    QML_ELEMENT

public:
    AlbumsQuery(QObject* parent = nullptr);

    void reload() override;
    void fetchMore(qint32) override;
};

class AlbumQuery : public QueryList, public QueryExtra<model::AlbumSongListModel, AlbumQuery> {
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
