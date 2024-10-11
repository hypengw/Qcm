#pragma once

#include <map>
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

    DECLARE_PROPERTY(ItemId, userId, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(QString, token, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(QString, nickname, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(QString, avatarUrl, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(QUrl, server, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(QUrl, session_file, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(Extra, extra, NOTIFY_NAME(infoChanged))
    Q_SIGNAL void infoChanged();

    void             insert(const ItemId&);
    Q_INVOKABLE bool contains(const ItemId&) const;

    Q_SIGNAL void query();

private:
    class Private;
    C_DECLARE_PRIVATE(UserAccount, d_ptr);
};

} // namespace model
} // namespace qcm
