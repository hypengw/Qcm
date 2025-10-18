#pragma once

#include <QQmlEngine>

#include "kstore/qt/gadget_model.hpp"
#include "Qcm/query/query.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{

class MixesQuery : public QueryList, public QueryExtra<model::MixListModel, MixesQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<qcm::msg::filter::MixFilter> filters READ filters WRITE setFilters NOTIFY
                   filtersChanged FINAL)
public:
    MixesQuery(QObject* parent = nullptr);
    auto filters() const -> const QList<msg::filter::MixFilter>&;
    void setFilters(const QList<msg::filter::MixFilter>&);

    void reload() override;
    void fetchMore(qint32) override;

    Q_SIGNAL void filtersChanged();

private:
    QList<msg::filter::MixFilter> m_filters;
};

class MixQuery : public Query, public QueryExtra<model::MixStoreItem, MixQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)

public:
    MixQuery(QObject* parent = nullptr);
    void reload() override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

} // namespace qcm
