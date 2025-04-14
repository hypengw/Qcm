#pragma once

#include <QtQml/QQmlEngine>
#include "Qcm/backend.hpp"
#include "Qcm/util/async.hpp"

namespace qcm::query
{
class ProviderMetasQuery : public QAsyncResultT<msg::GetProviderMetasRsp> {
    Q_OBJECT
    QML_ELEMENT
public:
    ProviderMetasQuery(QObject* parent = nullptr);
    void reload() override;
};

class AddProviderQuery : public QAsyncResultT<msg::AddProviderRsp> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(msg::AddProviderReq req READ req WRITE setReq NOTIFY reqChanged FINAL)
    Q_PROPERTY(QString failed READ failed NOTIFY failedChanged FINAL)
public:
    AddProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto req() -> msg::AddProviderReq&;
    void setReq(msg::AddProviderReq&);

    auto failed() const -> const QString&;
    void setFailed(QStringView);

    Q_SIGNAL void reqChanged();
    Q_SIGNAL void failedChanged();

private:
    msg::AddProviderReq m_req;
    QString m_failed;
};
} // namespace qcm::query