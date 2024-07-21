#include "qcm_interface/model.h"

#include "qcm_interface/model/user_account.h"

namespace qcm::model
{
UserAccount::UserAccount(QObject* parent) { this->setParent(parent); }
UserAccount::~UserAccount() {}
} // namespace qcm::model