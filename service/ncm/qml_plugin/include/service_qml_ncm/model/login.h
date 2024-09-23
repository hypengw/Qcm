#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/login.h"
#include "qcm_interface/global.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class Login : public QObject {
    Q_OBJECT
public:
    Login(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::Login;

    READ_PROPERTY(qint32, code, m_code, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_code, in.code);
        emit infoChanged();

        if (o.m_code) {
            auto user = new model::UserAccount(this);
            user->set_userId(convert_from<ItemId>(ncm::model::UserId {}));
            Global::instance()->user_model()->check_user(user);
        }
    }

signals:
    void infoChanged();
};
static_assert(modelable<Login, ncm::api::Login>);

} // namespace model

using LoginQuerier_base = ApiQuerier<ncm::api::Login, model::Login>;
class LoginQuerier : public LoginQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    LoginQuerier(QObject* parent = nullptr): LoginQuerier_base(parent) {}

    FORWARD_PROPERTY(QString, username, username)
    FORWARD_PROPERTY(QString, password, password_md5)
};
} // namespace qcm
