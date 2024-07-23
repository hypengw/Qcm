#pragma once

#include <QObject>
#include "qcm_interface/model.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class QCM_INTERFACE_API UserAccount : public Model<UserAccount, QObject> {
    Q_OBJECT
    DECLARE_MODEL()
public:
    UserAccount(QObject* parent = nullptr);
    ~UserAccount();

    DECLARE_PROPERTY(ItemId, userId, infoChanged)
    DECLARE_PROPERTY(QString, nickname, infoChanged)
    DECLARE_PROPERTY(QString, avatarUrl, infoChanged)

Q_SIGNALS:
    void infoChanged();
};

} // namespace model
} // namespace qcm
