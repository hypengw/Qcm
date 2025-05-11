#pragma once

#include <QQmlEngine>

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/query/query.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{

class MixesQuery : public QueryList, public QueryExtra<model::MixListModel, MixesQuery> {
    Q_OBJECT
    QML_ELEMENT
public:
    MixesQuery(QObject* parent = nullptr);
    void reload() override;
    void fetchMore(qint32) override;
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
