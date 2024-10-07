#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/qrcode_login.h"

namespace qcm
{

namespace model
{

class QrcodeLogin : public QObject {
    Q_OBJECT
public:
    QrcodeLogin(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::QrcodeLogin;

    READ_PROPERTY(qint32, code, m_code, infoChanged)
    READ_PROPERTY(QString, message, m_message, infoChanged)
    READ_PROPERTY(QString, nickname, m_nickname, infoChanged)
    READ_PROPERTY(QString, avatarUrl, m_avatarUrl, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_code, in.code);
        convert(o.m_message, in.message);
        convert(o.m_nickname, in.nickname);
        convert(o.m_avatarUrl, in.avatarUrl);
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<QrcodeLogin, ncm::api::QrcodeLogin>);

} // namespace model

using QrcodeLoginQuerier_base = ApiQuerier<ncm::api::QrcodeLogin, model::QrcodeLogin>;
class QrcodeLoginQuerier : public QrcodeLoginQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    QrcodeLoginQuerier(QObject* parent = nullptr): QrcodeLoginQuerier_base(parent) {}

    FORWARD_PROPERTY(QString, key, key)
};
} // namespace qcm
