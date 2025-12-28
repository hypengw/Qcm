#pragma once

#include <QQmlEngine>
#include "Qcm/query/query.hpp"
#include "Qcm/model/store_item.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{

class SyncQuery : public Query, public QueryExtra<msg::SyncRsp, SyncQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(
        qcm::model::ItemId providerId READ providerId WRITE setProviderId NOTIFY providerIdChanged)
public:
    SyncQuery(QObject* parent = nullptr);
    void reload() override;
    auto providerId() const -> model::ItemId;
    void setProviderId(const model::ItemId& v);

    Q_SIGNAL void providerIdChanged();

private:
    model::ItemId m_provider_id;
};

class SyncItemQuery : public Query, public QueryExtra<msg::Rsp, SyncItemQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    SyncItemQuery(QObject* parent = nullptr);
    void reload() override;
    auto itemId() const -> model::ItemId;
    void setItemId(const model::ItemId& v);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

} // namespace qcm