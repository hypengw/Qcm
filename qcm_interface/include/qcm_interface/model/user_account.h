#pragma once

#include <QQmlEngine>
#include "qcm_interface/model.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class UserAccount : public Model<UserAccount, QObject> {
    Q_OBJECT
    DECLARE_MODEL()
public:
    UserAccount(QObject* parent = nullptr) { this->setParent(parent); }

    DECLARE_PROPERTY(UserId, userId, infoChanged)
    DECLARE_PROPERTY(QString, nickname, infoChanged)
    DECLARE_PROPERTY(QString, avatarUrl, infoChanged)

Q_SIGNALS:
    void infoChanged();
};

} // namespace model
} // namespace qcm