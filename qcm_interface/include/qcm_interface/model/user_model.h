#pragma once

#include <QQmlEngine>
#include "core/core.h"
#include "qcm_interface/export.h"
#include "qcm_interface/async.h"
#include "meta_model/qobjectlistmodel.h"
#include "json_helper/helper.h"
#include "qcm_interface/model/user_account.h"

namespace qcm
{

namespace model
{
class UserAccount;
}

class QCM_INTERFACE_API UserModel
    : public meta_model::detail::QObjectListModel<model::UserAccount> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::UserAccount* activeUser READ active_user WRITE set_active_user NOTIFY
                   activeUserChanged)
public:
    UserModel(QObject* parent = nullptr);
    ~UserModel();

    auto find_by_url(const QUrl&) const -> model::UserAccount*;
    auto active_user() const -> model::UserAccount*;

public Q_SLOTS:
    void add_user(model::UserAccount* user);
    void delete_user(const model::ItemId& user_id);
    void set_active_user(model::UserAccount* user);

Q_SIGNALS:
    void activeUserChanged();
    void checkResultChanged();

private:
    class Private;
    C_DECLARE_PRIVATE(UserModel, d_ptr);
};
} // namespace qcm
DECLARE_JSON_SERIALIZER(qcm::UserModel);