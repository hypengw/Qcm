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

    Q_PROPERTY(msg::AuthProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
    Q_PROPERTY(QString failed READ failed NOTIFY failedChanged FINAL)
public:
    AuthProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto req() -> msg::AuthProviderReq&;
    void setReq(msg::AuthProviderReq&);

    auto failed() const -> const QString&;
    void setFailed(QStringView);

    Q_SIGNAL void reqChanged();
    Q_SIGNAL void failedChanged();

private:
    msg::AuthProviderReq m_req;
    QString              m_failed;
};

class AddProviderQuery : public Query, public QueryExtra<msg::Rsp, AddProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(msg::AddProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
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
        model::ItemId providerId READ providerId WRITE setProviderId NOTIFY providerIdChanged FINAL)
    Q_PROPERTY(msg::UpdateProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
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

    Q_PROPERTY(msg::DeleteProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
public:
    DeleteProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto req() -> msg::DeleteProviderReq&;
    void setReq(msg::DeleteProviderReq&);

    Q_SIGNAL void reqChanged();

private:
    msg::DeleteProviderReq m_req;
};

class ReplaceProviderQuery : public Query, public QueryExtra<msg::Rsp, ReplaceProviderQuery> {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        model::ItemId providerId READ providerId WRITE setProviderId NOTIFY providerIdChanged FINAL)
    Q_PROPERTY(msg::ReplaceProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
public:
    ReplaceProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto providerId() const -> model::ItemId;
    void setProviderId(const model::ItemId&);

    auto req() -> msg::ReplaceProviderReq&;
    void setReq(msg::ReplaceProviderReq&);

    Q_SIGNAL void reqChanged();

private:
    msg::ReplaceProviderReq m_req;
};

class CreateTmpProviderQuery
    : public Query,
      public QueryExtra<msg::CreateTmpProviderRsp, CreateTmpProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(msg::CreateTmpProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
public:
    CreateTmpProviderQuery(QObject* parent = nullptr);
    ~CreateTmpProviderQuery();
    void reload() override;

    auto req() -> msg::CreateTmpProviderReq&;
    void setReq(msg::CreateTmpProviderReq&);

    Q_SIGNAL void reqChanged();

private:
    msg::CreateTmpProviderReq m_req;
};

class DeleteTmpProviderQuery : public Query, public QueryExtra<msg::Rsp, DeleteTmpProviderQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(msg::DeleteTmpProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
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