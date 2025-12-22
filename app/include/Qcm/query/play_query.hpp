#pragma once

#include <QQmlEngine>
#include "Qcm/query/query.hpp"
#include "Qcm/model/store_item.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{
class PlayQuery : public Query {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    PlayQuery(QObject* parent = nullptr);
    void reload() override;

    auto          itemId() const -> model::ItemId;
    void          setItemId(model::ItemId);
    Q_SIGNAL void itemIdChanged(model::ItemId);

private:
    model::ItemId m_item_id;
};

class PlayAllQuery : public Query {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<qcm::msg::filter::AlbumFilter> filters READ filters WRITE setFilters NOTIFY
                   filtersChanged FINAL)

public:
    PlayAllQuery(QObject* parent = nullptr);
    void reload() override;

    auto          filters() const -> const QList<msg::filter::AlbumFilter>&;
    void          setFilters(const QList<msg::filter::AlbumFilter>&);
    Q_SIGNAL void filtersChanged();

private:
    QList<msg::filter::AlbumFilter> m_filters;
};

} // namespace qcm