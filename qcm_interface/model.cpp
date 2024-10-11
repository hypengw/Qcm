#include "qcm_interface/model.h"

#include "qcm_interface/model/user_account.h"
#include "qcm_interface/model/app_info.h"
#include "qcm_interface/model/busy_info.h"
#include "qcm_interface/model/plugin_info.h"
#include "qcm_interface/model/user_model.h"
#include "qcm_interface/model/page.h"
#include "qcm_interface/model/session.h"
#include "qcm_interface/model/router_msg.h"
#include "qcm_interface/global.h"
#include "asio_helper/basic.h"

#include "json_helper/helper.inl"

namespace qcm::model
{
class UserAccount::Private {
public:
    Private(UserAccount* p): collection(p) {}
    // id, type
    std::unordered_map<std::size_t, std::size_t> items;
    std::unordered_map<std::size_t, QString>     type_map;

    UserAccountCollection collection;
};

UserAccount::UserAccount(QObject* parent): d_ptr(make_up<Private>(this)) {
    this->setParent(parent);
    connect(this, &UserAccount::query, this, [this] {
        asio::co_spawn(
            Global::instance()->qexecutor(),
            [this] -> asio::awaitable<void> {
                C_D(UserAccount);
                auto sql = Global::instance()->get_collection_sql();
                auto res = co_await sql->select_id(userId());

                d->items.clear();
                for (auto& el : res) {
                    insert(el);
                }
                co_return;
            },
            helper::asio_detached_log);
    });
}
UserAccount::~UserAccount() {}

void UserAccount::insert(const ItemId& item) {
    C_D(UserAccount);
    auto hash      = std::hash<ItemId> {}(item);
    auto type_hash = std::hash<QString> {}(item.type());
    d->type_map.insert_or_assign(type_hash, item.type());
    auto [_, ok] = d->items.insert_or_assign(hash, type_hash);
    if (ok) {
        collectionChanged();
    }
}
void UserAccount::remove(const ItemId& item) {
    C_D(UserAccount);
    auto hash = std::hash<ItemId> {}(item);
    if (d->items.erase(hash)) {
        collectionChanged();
    }
}
auto UserAccount::contains(const ItemId& id) const -> bool {
    C_D(const UserAccount);
    auto hash = std::hash<ItemId> {}(id);
    return d->items.contains(hash);
}

auto UserAccount::collection() const -> const UserAccountCollection& {
    C_D(const UserAccount);
    return d->collection;
}
UserAccountCollection::UserAccountCollection(UserAccount* p): m_parent(p) {}
UserAccountCollection::~UserAccountCollection() {}
bool UserAccountCollection::contains(const ItemId& id) const { return m_parent->contains(id); }

AppInfo::AppInfo() {
    set_id(APP_ID);
    set_name(APP_NAME);
    set_version(APP_VERSION);
    set_author(APP_AUTHOR);
    set_summary(APP_SUMMARY);
}
AppInfo::~AppInfo() {}
BusyInfo::BusyInfo(QObject* parent) {
    setParent(parent);
    set_load_session(false);
}
BusyInfo::~BusyInfo() {}

PluginInfo::PluginInfo() {}
PluginInfo::~PluginInfo() {}

Page::Page() {
    set_cache(false);
    set_primary(false);
}
Page::~Page() {}

Session::Session(QObject* parent) {
    this->setParent(parent);
    set_user(new model::UserAccount(this));
    set_valid(false);
}
Session::~Session() {}

auto Session::client() const -> std::optional<Client> { return m_client; }
void Session::set_client(std::optional<Client> c) { m_client = c; }

RouteMsg::RouteMsg() {}
RouteMsg::~RouteMsg() {}

} // namespace qcm::model

IMPL_JSON_SERIALIZER_FROM(QString) {
    std::string s;
    j.get_to(s);
    t = QString::fromStdString(s);
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

IMPL_JSON_SERIALIZER_FROM(QVariantMap) {
    Q_UNUSED(j)
    Q_UNUSED(t)
}
IMPL_JSON_SERIALIZER_TO(QVariantMap) {
    Q_UNUSED(j)
    Q_UNUSED(t)
}

IMPL_CONVERT(std::string, qcm::model::ItemId) { out = in.toUrl().toString().toStdString(); }
