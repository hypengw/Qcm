#include "qcm_interface/model.h"

#include "qcm_interface/model/user_account.h"
#include "qcm_interface/model/app_info.h"
#include "qcm_interface/model/plugin_info.h"
#include "qcm_interface/model/user_model.h"
#include "qcm_interface/model/page.h"
#include "qcm_interface/model/session.h"

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

Page::Page() {
    set_cache(false);
    set_primary(false);
}
Page::~Page() {}

Session::Session(QObject* parent) {
    this->setParent(parent);
    set_user(nullptr);
}
Session::~Session() {}

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
    if (j.contains("active_user")) {
        auto user_idx = j.at("active_user").get<i64>();
        if (user_idx < (i64)t.size()) {
            t.set_active_user(t.at(user_idx));
        }
    }
}

IMPL_JSON_SERIALIZER_TO(qcm::UserModel) {
    qcm::json::njson j_user;
    for (auto el : t) {
        j_user.push_back(*el);
    }
    j["users"] = j_user;
    if (t.active_user() != nullptr) {
        if (auto it = t.find(t.active_user()); it != t.end()) {
            j["active_user"] = std::distance(t.begin(), it);
        }
    }
}

IMPL_CONVERT(std::string, qcm::model::ItemId) { out = in.toUrl().toString().toStdString(); }
