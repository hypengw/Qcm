#pragma once

#include <QtCore/QAbstractListModel>
#include <QQmlEngine>
#include "core/core.h"
#include "qcm_interface/export.h"
#include "meta_model/qobjectlistmodel.h"
#include "json_helper/helper.h"

namespace qcm
{

namespace model
{
class UserAccount;
}

class QCM_INTERFACE_API UserModel : public meta_model::QObjectListModel {
    Q_OBJECT
    QML_ELEMENT
public:
    UserModel(QObject* parent = nullptr);
    ~UserModel();

public Q_SLOTS:
    void add_user(model::UserAccount*);
    void delete_user();

private:
    class Private;
    C_DECLARE_PRIVATE(UserModel, d_ptr);
};
} // namespace qcm
DECLARE_JSON_SERIALIZER(qcm::UserModel);