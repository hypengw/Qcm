#include "qcm_interface/model.h"

#include "qcm_interface/model/user_account.h"
#include "qcm_interface/model/app_info.h"

namespace qcm::model
{
UserAccount::UserAccount(QObject* parent) { this->setParent(parent); }
UserAccount::~UserAccount() {}
AppInfo::AppInfo() { 
    set_id(APP_ID); 
    set_version(APP_VERSION);
}
AppInfo::~AppInfo() {}
} // namespace qcm::model