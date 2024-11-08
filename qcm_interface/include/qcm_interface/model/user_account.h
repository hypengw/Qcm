#pragma once

#include <map>
#include <QObject>
#include <QPointer>
#include "qcm_interface/model.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class UserAccount;
class UserAccountCollection : public QObject {
    Q_OBJECT
public:
    UserAccountCollection(UserAccount*);
    ~UserAccountCollection();

    Q_INVOKABLE bool contains(const ItemId&) const;

private:
    QPointer<UserAccount> m_parent;
};
class QCM_INTERFACE_API UserAccount : public Model<UserAccount, QObject> {
    Q_OBJECT
    DECLARE_MODEL()

    Q_PROPERTY(qcm::model::UserAccountCollection* collection READ collection NOTIFY
                   collectionChanged FINAL)
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

    void insert(const ItemId&);
    void insert(std::span<const ItemId>);
    void remove(const ItemId&);
    bool contains(const ItemId&) const;

    auto collection() const -> UserAccountCollection*;

    Q_SIGNAL void query(QString type = {});
    Q_SIGNAL void collectionChanged();

private:
    class Private;
    C_DECLARE_PRIVATE(UserAccount, d_ptr);
};

} // namespace model
} // namespace qcm
