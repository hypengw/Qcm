#pragma once

#include <QQmlEngine>

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/query/query.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{
class ArtistsQuery : public query::QueryList<model::ArtistListModel> {
    Q_OBJECT
    QML_ELEMENT
public:
    ArtistsQuery(QObject* parent = nullptr);
    void reload() override;
    void fetchMore(qint32) override;
};

class ArtistQuery : public query::Query<model::ArtistStoreItem> {
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

} // namespace qcm
