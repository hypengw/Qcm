#include "qcm_interface/model.h"

#include "qcm_interface/model/user_account.h"
#include "qcm_interface/model/app_info.h"
#include "qcm_interface/model/plugin_info.h"
#include "qcm_interface/model/user_model.h"

#include "json_helper/helper.inl"

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

PluginInfo::PluginInfo() {}
PluginInfo::~PluginInfo() {}

} // namespace qcm::model

IMPL_JSON_SERIALIZER_FROM(QString) {
    std::string s;
    j.get_to(s);
    t.fromStdString(s);
}
IMPL_JSON_SERIALIZER_TO(QString) { j = t.toStdString(); }
IMPL_JSON_SERIALIZER_FROM(QUrl) {
    std::string s;
    j.get_to(s);
    t = QString::fromStdString(s);
}
IMPL_JSON_SERIALIZER_TO(QUrl) { j = t.toString().toStdString(); }

IMPL_JSON_SERIALIZER_FROM(qcm::model::ItemId) {
    std::string url;
    j.get_to(url);
    t.set_url(QUrl(QString::fromStdString(url)));
}
IMPL_JSON_SERIALIZER_TO(qcm::model::ItemId) {
    auto url = t.toUrl().toString().toStdString();
    j        = url;
}

IMPL_JSON_SERIALIZER_FROM(qcm::UserModel) {
    if (j.contains("users")) {
        for (auto& el : j.at("users")) {
            auto u = make_up<qcm::model::UserAccount>();
            el.get_to(*u);
            t.add_user(u.release());
        }
    }
}

IMPL_JSON_SERIALIZER_TO(qcm::UserModel) {
    qcm::json::njson j_user;
    for (auto el : t) {
        if (auto u = qobject_cast<qcm::model::UserAccount*>(el)) {
            j_user.push_back(*u);
        }
    }
    j["users"] = j_user;
}

IMPL_CONVERT(std::string, qcm::model::ItemId) { out = in.toUrl().toString().toStdString(); }
