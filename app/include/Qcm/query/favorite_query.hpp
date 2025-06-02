#pragma once

#include <QQmlEngine>
#include "Qcm/query/query.hpp"
#include "Qcm/backend_msg.hpp"

namespace qcm
{
class SetFavoriteQuery : public Query, public QueryExtra<msg::Rsp, SetFavoriteQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged FINAL)
    Q_PROPERTY(bool favorite READ favorite WRITE setFavorite NOTIFY favoriteChanged FINAL)
public:
    SetFavoriteQuery(QObject* parent = nullptr);
    void reload() override;

    auto favorite() const -> bool;
    void setFavorite(bool);
    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void favoriteChanged();
    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
    bool m_favorite;
};
} // namespace qcm