module;
#include "QExtra/macro_qt.hpp"



#ifdef Q_MOC_RUN
#include "Qcm/query/provider_query.moc"
#endif

export module qcm:query.provider;
export import :query.query;
export import :msg;

namespace qcm
{

export class ProviderMetasQuery : public Query,
                           public QueryExtra<msg::GetProviderMetasRsp, ProviderMetasQuery> {
    Q_OBJECT
    QML_ELEMENT
public:
    ProviderMetasQuery(QObject* parent = nullptr);
    void reload() override;
};

export class AuthProviderQuery : public Query, public QueryExtra<msg::AuthProviderRsp, AuthProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::AuthProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
    Q_PROPERTY(
        QString tmpProvider READ tmpProvider WRITE setTmpProvider NOTIFY tmpProviderChanged FINAL)
    Q_PROPERTY(QString failed READ failed NOTIFY failedChanged FINAL)
public:
    AuthProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto req() -> msg::AuthProviderReq&;
    void setReq(msg::AuthProviderReq&);
    auto tmpProvider() const -> QString;
    void setTmpProvider(const QString&);

    auto failed() const -> const QString&;
    void setFailed(QStringView);

    Q_SIGNAL void reqChanged();
    Q_SIGNAL void failedChanged();
    Q_SIGNAL void tmpProviderChanged();

private:
    msg::AuthProviderReq m_req;
    QString              m_failed;
    QString              m_tmp_provider;
};

export class AddProviderQuery : public Query, public QueryExtra<msg::Rsp, AddProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::AddProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
public:
    AddProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto req() -> msg::AddProviderReq&;
    void setReq(msg::AddProviderReq&);

    Q_SIGNAL void reqChanged();

private:
    msg::AddProviderReq m_req;
};

export class ProviderSettingsQuery : public Query {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qcm::msg::id::ProviderId providerId READ providerId WRITE setProviderId NOTIFY
                   providerIdChanged FINAL)
    Q_PROPERTY(QVariantList fields READ fields NOTIFY settingsChanged FINAL)
    Q_PROPERTY(QVariantMap values READ values NOTIFY settingsChanged FINAL)
    Q_PROPERTY(quint64 revision READ revision NOTIFY settingsChanged FINAL)
public:
    ProviderSettingsQuery(QObject* parent = nullptr);
    void          reload() override;
    auto          providerId() const -> msg::id::ProviderId { return m_provider_id; }
    void          setProviderId(const msg::id::ProviderId&);
    auto          fields() const -> QVariantList { return m_fields; }
    auto          values() const -> QVariantMap { return m_values; }
    auto          revision() const -> quint64 { return m_revision; }
    Q_SIGNAL void providerIdChanged();
    Q_SIGNAL void settingsChanged();

private:
    msg::id::ProviderId m_provider_id;
    QVariantList  m_fields;
    QVariantMap   m_values;
    quint64       m_revision {};
};

export class UpdateProviderQuery : public Query,
                                   public QueryExtra<msg::UpdateProviderRsp, UpdateProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::id::ProviderId providerId READ providerId WRITE setProviderId NOTIFY
                   providerIdChanged FINAL)
    Q_PROPERTY(qcm::msg::UpdateProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
    Q_PROPERTY(QVariantMap fieldErrors READ fieldErrors NOTIFY fieldErrorsChanged FINAL)
public:
    UpdateProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto providerId() const -> msg::id::ProviderId;
    void setProviderId(const msg::id::ProviderId&);

    auto          req() -> msg::UpdateProviderReq&;
    void          setReq(const msg::UpdateProviderReq&);
    auto          fieldErrors() const -> QVariantMap { return m_field_errors; }
    Q_SIGNAL void fieldErrorsChanged();

    Q_SIGNAL void reqChanged();
    Q_SIGNAL void providerIdChanged();

private:
    msg::UpdateProviderReq m_req;
    QVariantMap            m_field_errors;
    msg::id::ProviderId    m_provider_id;
};

export class DeleteProviderQuery : public Query, public QueryExtra<msg::Rsp, DeleteProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::id::ProviderId providerId READ providerId WRITE setProviderId NOTIFY
                   providerIdChanged FINAL)
public:
    DeleteProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto providerId() const -> msg::id::ProviderId;
    void setProviderId(const msg::id::ProviderId&);

    Q_SIGNAL void reqChanged();
    Q_SIGNAL void providerIdChanged();

private:
    msg::id::ProviderId    m_provider_id;
    msg::DeleteProviderReq m_req;
};

export class ReplaceProviderQuery : public Query, public QueryExtra<msg::Rsp, ReplaceProviderQuery> {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qcm::msg::id::ProviderId providerId READ providerId WRITE setProviderId NOTIFY
                   providerIdChanged FINAL)
    Q_PROPERTY(
        QString tmpProvider READ tmpProvider WRITE setTmpProvider NOTIFY tmpProviderChanged FINAL)
public:
    ReplaceProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto providerId() const -> msg::id::ProviderId;
    void setProviderId(const msg::id::ProviderId&);
    auto tmpProvider() const -> QString;
    void setTmpProvider(const QString&);

    Q_SIGNAL void tmpProviderChanged();
    Q_SIGNAL void providerIdChanged();

private:
    QString       m_tmp_provider;
    msg::id::ProviderId m_provider_id;
};

export class CreateTmpProviderQuery
    : public Query,
      public QueryExtra<msg::CreateTmpProviderRsp, CreateTmpProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString typeName READ typeName WRITE setTypeName NOTIFY typeNameChanged FINAL)
    Q_PROPERTY(qcm::msg::id::ProviderId networkProviderId READ networkProviderId WRITE
                   setNetworkProviderId NOTIFY networkProviderIdChanged FINAL)
public:
    CreateTmpProviderQuery(QObject* parent = nullptr);
    ~CreateTmpProviderQuery();
    void reload() override;

    auto typeName() const -> QString;
    void setTypeName(const QString&);

    auto          networkProviderId() const -> msg::id::ProviderId { return m_network_provider_id; }
    void          setNetworkProviderId(const msg::id::ProviderId&);
    Q_SIGNAL void networkProviderIdChanged();
    Q_SIGNAL void typeNameChanged();

private:
    QString m_type_name;
    msg::id::ProviderId m_network_provider_id;
};

export class DeleteTmpProviderQuery : public Query, public QueryExtra<msg::Rsp, DeleteTmpProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::DeleteTmpProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
public:
    DeleteTmpProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto req() -> msg::DeleteTmpProviderReq&;
    void setReq(msg::DeleteTmpProviderReq&);

    Q_SIGNAL void reqChanged();

private:
    msg::DeleteTmpProviderReq m_req;
};

} 

