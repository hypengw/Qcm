#pragma once

#include <QtQml/QQmlEngine>
#include "Qcm/backend.hpp"
#include "qcm_interface/async.h"

namespace qcm::query
{
class ProviderMetasQuery : public QAsyncResultT<msg::GetProviderMetasRsp> {
    Q_OBJECT
    QML_ELEMENT
public:
    ProviderMetasQuery(QObject* parent = nullptr);
    void reload() override;
};

class AddProviderQuery : public QAsyncResultT<msg::Rsp> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(msg::AddProviderReq req READ req WRITE setReq NOTIFY reqChanged)
public:
    AddProviderQuery(QObject* parent = nullptr);
    void reload() override;

    auto req() -> msg::AddProviderReq&;
    void setReq(msg::AddProviderReq&);

    Q_SIGNAL void reqChanged();

private:
    msg::AddProviderReq m_req;
};
} // namespace qcm::query