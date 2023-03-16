#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/user_account.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class UserAccount : public QObject {
    Q_OBJECT
public:
    UserAccount(QObject* parent = nullptr): QObject(parent) {}

    READ_PROPERTY(UserId, userId, m_userId, infoChanged)
    READ_PROPERTY(QString, nickname, m_nickname, infoChanged)
    READ_PROPERTY(QString, avatarUrl, m_avatarUrl, infoChanged)

    using out_type = ncm::api_model::UserAccount;

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;

        const auto& profile = in.profile.value_or(ncm::model::UserAccountProfile {});
        CONVERT_PROPERTY(o.m_userId, profile.userId);
        CONVERT_PROPERTY(o.m_nickname, profile.nickname);
        CONVERT_PROPERTY(o.m_avatarUrl, profile.avatarUrl);
        ERROR_LOG("id: {}", o.m_userId.id);
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<UserAccount, ncm::api::UserAccount>);

} // namespace model

using UserAccountQuerier_base = ApiQuerier<ncm::api::UserAccount, model::UserAccount>;
class UserAccountQuerier : public UserAccountQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    UserAccountQuerier(QObject* parent = nullptr): UserAccountQuerier_base(parent) {}
};

} // namespace qcm
