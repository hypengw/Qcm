#include "Qcm/model/app_info.hpp"

namespace qcm::model
{
AppInfo::AppInfo() {
    this->name    = APP_NAME;
    this->id      = APP_ID;
    this->author  = APP_AUTHOR;
    this->summary = APP_SUMMARY;
    this->version = APP_VERSION;
}
AppInfo::~AppInfo() {}
} // namespace qcm::model

#include <Qcm/model/moc_app_info.cpp>