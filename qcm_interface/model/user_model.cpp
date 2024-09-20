#include "qcm_interface/model/user_model.h"
#include "qcm_interface/model/user_account.h"

namespace qcm
{

class UserModel::Private {
public:
};

UserModel::UserModel(QObject* parent)
    : meta_model::QObjectListModel(model::UserAccount::staticMetaObject, true, parent),
      d_ptr(make_up<Private>()) {}

UserModel::~UserModel() {}

void UserModel::add_user(model::UserAccount* user) { insert(rowCount(), user); }
void UserModel::delete_user() {}

} // namespace qcm