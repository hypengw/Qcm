#include "qcm_interface/model.h"

#include "qcm_interface/model/user_account.h"
#include "qcm_interface/model/app_info.h"

namespace qcm::model
{
UserAccount::UserAccount(QObject* parent) { this->setParent(parent); }
UserAccount::~UserAccount() {}
AppInfo::AppInfo() {
    set_id(APP_ID);
    set_name(APP_NAME);
    set_version(APP_VERSION);
    set_author(APP_AUTHOR);
    set_summary(APP_SUMMARY);
}
AppInfo::~AppInfo() {}
} // namespace qcm::model

IMPL_CONVERT(std::string, qcm::model::ItemId) { out = in.toUrl().toString().toStdString(); }
