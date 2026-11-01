module;
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#include "Qcm/query/sync_query.moc"
#endif

export module qcm:query.sync;
export import :query.query;
export import :model.store_item;

namespace qcm
{

export class SyncQuery : public Query, public QueryExtra<msg::SyncRsp, SyncQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::id::ProviderId providerId READ providerId WRITE setProviderId NOTIFY
                   providerIdChanged)
public:
    SyncQuery(QObject* parent = nullptr);
    void reload() override;
    auto providerId() const -> msg::id::ProviderId;
    void setProviderId(const msg::id::ProviderId& v);

    Q_SIGNAL void providerIdChanged();

private:
    msg::id::ProviderId m_provider_id;
};

export class SyncItemQuery : public Query, public QueryExtra<msg::Rsp, SyncItemQuery> {
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

} 

