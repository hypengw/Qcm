#include "qcm_interface/model.h"

#include "qcm_interface/model/album.h"
#include "qcm_interface/model/user_account.h"
#include "qcm_interface/model/app_info.h"
#include "qcm_interface/model/busy_info.h"
#include "qcm_interface/model/plugin_info.h"
#include "qcm_interface/model/user_model.h"
#include "qcm_interface/model/page.h"
#include "qcm_interface/model/session.h"
#include "qcm_interface/model/router_msg.h"
#include "qcm_interface/global.h"
#include "qcm_interface/notifier.h"
#include "qcm_interface/sql/meta_sql.h"
#include "qcm_interface/model/query_model.h"
#include "asio_helper/basic.h"

#include "json_helper/helper.inl"

namespace qcm::model
{
class UserAccount::Private {
public:
    Private(UserAccount* p): collection(new UserAccountCollection(p)) {}
    // id, type
    std::unordered_map<std::size_t, std::size_t> items;
    std::unordered_map<std::size_t, QString>     type_map;

    UserAccountCollection* collection;
};

UserAccount::UserAccount(QObject* parent): d_ptr(make_up<Private>(this)) {
    this->setParent(parent);

    connect(Notifier::instance(),
            &Notifier::collection_synced,
            this,
            [this](enums::CollectionType type, model::ItemId userId) {
                if (userId == this->userId()) {
                    this->query(convert_from<QString>(type));
                }
            });
    connect(this, &UserAccount::query, this, [this](QString type) {
        asio::co_spawn(
            Global::instance()->qexecutor(),
            [this, type] -> asio::awaitable<void> {
                C_D(UserAccount);
                auto sql = Global::instance()->get_collection_sql();
                auto res = co_await sql->select_id(userId(), type);

                if (type.isEmpty()) {
                    d->items.clear();
                } else {
                    auto type_hash = std::hash<QString> {}(type);
                    std::erase_if(d->items, [type_hash](const auto& el) -> bool {
                        return el.second == type_hash;
                    });
                }
                insert(res);
                co_return;
            },
            helper::asio_detached_log_t {});
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

void UserAccount::insert(std::span<const ItemId> items) {
    C_D(UserAccount);
    for (auto& item : items) {
        auto hash      = std::hash<ItemId> {}(item);
        auto type_hash = std::hash<QString> {}(item.type());
        d->type_map.insert_or_assign(type_hash, item.type());
        d->items.insert_or_assign(hash, type_hash);
    }
    collectionChanged();
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

auto UserAccount::collection() const -> UserAccountCollection* {
    C_D(const UserAccount);
    return d->collection;
}
UserAccountCollection::UserAccountCollection(UserAccount* p): QObject(p), m_parent(p) {}
UserAccountCollection::~UserAccountCollection() {}
bool UserAccountCollection::contains(const ItemId& id) const {
    return m_parent && m_parent->contains(id);
}

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
    set_supportComment(false);
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
        std::optional<qcm::model::UserAccount*> active_user;
        std::optional<i64>                      active_user_id;
        if (j.contains("active_user")) {
            active_user_id = j.at("active_user").get<i64>();
        }

        i64 i = 0;
        for (auto& el : j.at("users")) {
            auto u = make_up<qcm::model::UserAccount>();
            el.get_to(*u);
            if (u->userId().valid()) {
                if (active_user_id == std::optional { i }) {
                    active_user = u.get();
                }
                t.add_user(u.release());
            }
            i++;
        }
        if (active_user) {
            t.set_active_user(active_user.value());
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
IMPL_JSON_SERIALIZER_FROM(QDateTime) {
    std::string text;
    j.get_to(text);
    t.fromString(QString::fromStdString(text), Qt::DateFormat::TextDate);
}
IMPL_JSON_SERIALIZER_TO(QDateTime) { j = t.toString(Qt::DateFormat::TextDate); }

IMPL_CONVERT(std::string, qcm::model::ItemId) { out = in.toUrl().toString().toStdString(); }

namespace
{
template<typename T>
auto generate_sql(std::string_view table, std::set<std::string_view> ignore = {},
                  std::string_view pre = {}) -> qcm::model::ModelSql {
    const auto&          meta = T::staticMetaObject;
    qcm::model::ModelSql out;
    for (int i = 0; i < meta.propertyCount(); i++) {
        std::string_view name = meta.property(i).name();
        if (ignore.contains(name)) continue;
        out.columns.emplace_back(name);
        out.idxs.emplace_back(i);
    }
    out.select = fmt::format(
        "{}{}",
        pre.empty() ? ""s : fmt::format("{}\n,", pre),
        fmt::join(std::views::transform(std::views::filter(out.columns,
                                                           [&ignore](const auto& el) -> bool {
                                                               return ! ignore.contains(el);
                                                           }),
                                        [table](const auto& el) {
                                            return fmt::format("{}.{}", table, el);
                                        }),
                  ",\n"));

    out.group_select = fmt::format(
        "{}{}",
        pre.empty() ? ""s : fmt::format("{}\n,", pre),
        fmt::join(std::views::transform(
                      std::views::filter(out.columns,
                                         [&ignore](const auto& el) -> bool {
                                             return ! ignore.contains(el);
                                         }),
                      [table](const auto& el) {
                          return fmt::format("GROUP_CONCAT({0}.{1}) AS group_{0}_{1}", table, el);
                      }),
                  ",\n"));

    return out;
}
#define IMPL_SQL_MODEL(type, table, ...)                                         \
    auto type::sql() -> const model::ModelSql& {                                 \
        static auto sql = generate_sql<type>(#table __VA_OPT__(, ) __VA_ARGS__); \
        return sql;                                                              \
    }
} // namespace
// select
namespace qcm
{

IMPL_SQL_MODEL(model::ArtistRefer, artist)
IMPL_SQL_MODEL(model::Artist, artist)
IMPL_SQL_MODEL(model::RadioRefer, radio)
IMPL_SQL_MODEL(model::Radio, radio)
IMPL_SQL_MODEL(model::AlbumRefer, album)
IMPL_SQL_MODEL(model::Album, album)
IMPL_SQL_MODEL(model::PlaylistRefer, playlist)
IMPL_SQL_MODEL(model::Playlist, playlist)

} // namespace qcm

namespace
{
template<>
auto generate_sql<qcm::query::Album>(std::string_view, std::set<std::string_view>, std::string_view)
    -> qcm::model::ModelSql {
    auto sql   = qcm::model::Album::sql();
    sql.select = fmt::format("{},\n{}", sql.select, qcm::model::ArtistRefer::sql().group_select);
    return sql;
}

} // namespace

namespace qcm
{
IMPL_SQL_MODEL(query::Album, album)

} // namespace qcm