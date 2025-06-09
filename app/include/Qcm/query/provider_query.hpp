#pragma once

#include <QtQml/QQmlEngine>
#include "Qcm/backend.hpp"
#include "Qcm/query/query.hpp"

namespace qcm
{
class ProviderMetasQuery : public Query,
                           public QueryExtra<msg::GetProviderMetasRsp, ProviderMetasQuery> {
    Q_OBJECT
    QML_ELEMENT
public:
    ProviderMetasQuery(QObject* parent = nullptr);
    void reload() override;
};

class AuthProviderQuery : public Query, public QueryExtra<msg::AuthProviderRsp, AuthProviderQuery> {
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

class AddProviderQuery : public Query, public QueryExtra<msg::Rsp, AddProviderQuery> {
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

class UpdateProviderQuery : public Query,
                            public QueryExtra<msg::UpdateProviderRsp, UpdateProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(
        qcm::model::ItemId providerId READ providerId WRITE setProviderId NOTIFY providerIdChanged FINAL)
    Q_PROPERTY(qcm::msg::UpdateProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
public:
    UpdateProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto providerId() const -> model::ItemId;
    void setProviderId(const model::ItemId&);

    auto req() -> msg::UpdateProviderReq&;
    void setReq(const msg::UpdateProviderReq&);

    Q_SIGNAL void reqChanged();
    Q_SIGNAL void providerIdChanged();

private:
    msg::UpdateProviderReq m_req;
    model::ItemId          m_provider_id;
};

class DeleteProviderQuery : public Query, public QueryExtra<msg::Rsp, DeleteProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId providerId READ providerId WRITE setProviderId NOTIFY providerIdChanged FINAL)
public:
    DeleteProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto providerId() const -> model::ItemId;
    void setProviderId(const model::ItemId&);

    Q_SIGNAL void reqChanged();
    Q_SIGNAL void providerIdChanged();

private:
    model::ItemId          m_provider_id;
    msg::DeleteProviderReq m_req;
};

class ReplaceProviderQuery : public Query, public QueryExtra<msg::Rsp, ReplaceProviderQuery> {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        qcm::model::ItemId providerId READ providerId WRITE setProviderId NOTIFY providerIdChanged FINAL)
    Q_PROPERTY(qcm::msg::ReplaceProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
public:
    ReplaceProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto providerId() const -> model::ItemId;
    void setProviderId(const model::ItemId&);

    auto req() -> msg::ReplaceProviderReq&;
    void setReq(msg::ReplaceProviderReq&);

    Q_SIGNAL void reqChanged();
    Q_SIGNAL void providerIdChanged();

private:
    msg::ReplaceProviderReq m_req;
    model::ItemId           m_provider_id;
};

class CreateTmpProviderQuery
    : public Query,
      public QueryExtra<msg::CreateTmpProviderRsp, CreateTmpProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString typeName READ typeName WRITE setTypeName NOTIFY typeNameChanged FINAL)
public:
    CreateTmpProviderQuery(QObject* parent = nullptr);
    ~CreateTmpProviderQuery();
    void reload() override;

    auto typeName() const -> QString;
    void setTypeName(const QString&);

    Q_SIGNAL void typeNameChanged();

private:
    QString m_type_name;
};

class DeleteTmpProviderQuery : public Query, public QueryExtra<msg::Rsp, DeleteTmpProviderQuery> {
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

} // namespace qcm