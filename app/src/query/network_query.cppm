module;
#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/query/network_query.moc"
#endif
export module qcm:query.network;
export import :query.query;
export import :msg;

namespace qcm
{
export class NetworkSettingsQuery : public Query {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList proxies READ proxies NOTIFY settingsChanged FINAL)
    Q_PROPERTY(quint64 revision READ revision NOTIFY settingsChanged FINAL)
public:
    NetworkSettingsQuery(QObject* parent = nullptr);
    void          reload() override;
    auto          proxies() const -> QVariantList { return m_proxies; }
    auto          revision() const -> quint64 { return m_revision; }
    Q_SIGNAL void settingsChanged();

private:
    QVariantList m_proxies;
    quint64      m_revision {};
};
export class UpdateNetworkSettingsQuery : public Query,
                                          public QueryExtra<msg::Rsp, UpdateNetworkSettingsQuery> {
    Q_OBJECT
    QML_ELEMENT
public:
    UpdateNetworkSettingsQuery(QObject* parent = nullptr);
    Q_INVOKABLE void save(const QVariantList& proxies, quint64 revision);
    void             reload() override;

private:
    msg::UpdateNetworkSettingsReq m_req;
};
} // namespace qcm
