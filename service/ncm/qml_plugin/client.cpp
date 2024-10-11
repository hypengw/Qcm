#include "service_qml_ncm/client.h"

#include <QUuid>
#include <asio/bind_executor.hpp>
#include "core/qstr_helper.h"

#include "qcm_interface/async.inl"
#include "qcm_interface/global.h"
#include "qcm_interface/model/session.h"

#include "service_qml_ncm/model.h"
#include "asio_helper/detached_log.h"
#include "asio_qt/qt_holder.h"

#include "ncm/api/feedback_weblog.h"
#include "ncm/api/user_account.h"
#include "ncm/api/logout.h"
#include "ncm/api/song_like.h"
#include "ncm/api/radio_like.h"
#include "ncm/client.h"

namespace
{

constexpr int        MIN_IMG_SIZE { 300 };
constexpr std::array IMG_DL_SIZES { 240, 480, 960, 1920 };

inline QSize get_down_size(const QSize& req) {
    usize req_size = std::sqrt(req.width() * req.height());
    auto  it       = std::lower_bound(IMG_DL_SIZES.begin(), IMG_DL_SIZES.end(), req_size);
    usize size     = it != IMG_DL_SIZES.end() ? *it : IMG_DL_SIZES.back();

    return req.scaled(size, size, Qt::AspectRatioMode::KeepAspectRatioByExpanding);
}

auto prepare_session(ncm::Client c, qcm::model::ItemId userId) -> asio::awaitable<void> {
    auto sql = qcm::Global::instance()->get_collection_sql();

    ncm::api::SongLike api;
    api.input.uid                       = convert_from<ncm::model::UserId>(userId);
    auto                            res = co_await c.perform(api);
    std::vector<qcm::model::ItemId> ids;
    if (res) {
        for (auto& id : res.value().ids) {
            auto item_id = ncm::to_ncm_id(ncm::model::IdType::Song, id);
            ids.push_back(item_id);
        }

        co_await sql->refresh(userId, "song", ids);

        qcm::Global::instance()->qsession()->user()->query();
    }
    co_return;
}

} // namespace

namespace ncm::impl
{
using ClientBase = qcm::ClientBase;

struct Client : public ClientBase {
    Client(ncm::Client ncm): ncm(ncm) {}
    ncm::Client ncm;
};

static auto get_client(ClientBase& c) -> ncm::Client* { return &static_cast<Client&>(c).ncm; }

static auto server_url(ClientBase& cbase, const qcm::model::ItemId& id) -> std::string {
    auto c = get_client(cbase);
    switch (UNWRAP(ncm_id_type(id))) {
    case ncm::model::IdType::Song: {
        return fmt::format("{}/#song?id={}", ncm::BASE_URL, id.id());
    }
    default: {
        return {};
    }
    }
}

static void play_state(ClientBase& cbase, qcm::enums::PlaybackState state,
                       qcm::model::ItemId itemId, qcm::model::ItemId sourceId, i64 played_second,
                       QVariantMap) {
    if (state == qcm::enums::PlaybackState::PausedState) return;
    auto c         = *get_client(cbase);
    auto state_old = c.prop("play_state");
    if (state == qcm::enums::PlaybackState::PlayingState) {
        if (state_old) {
            if (std::any_cast<qcm::enums::PlaybackState>(state_old.value()) == state) {
                return;
            }
        }
    }
    c.set_prop("play_state", state);

    auto                     ex = asio::make_strand(c.get_executor());
    ncm::api::FeedbackWeblog api;

    if (! helper::variant_convert(api.input.id, to_ncm_id(itemId))) {
        ERROR_LOG("");
        return;
    }

    api.input.act  = state == qcm::enums::PlaybackState::PlayingState
                         ? ncm::params::FeedbackWeblog::Action::Start
                         : ncm::params::FeedbackWeblog::Action::End;
    api.input.time = played_second;

    helper::variant_convert(api.input.sourceId, to_ncm_id(sourceId));

    asio::co_spawn(
        ex,
        [c, api]() mutable -> asio::awaitable<void> {
            co_await c.perform(api);
            co_return;
        },
        helper::asio_detached_log);
}

static auto router(ClientBase& cbase) -> rc<qcm::Router> {
    auto c          = *get_client(cbase);
    auto router_any = c.prop("router"sv);
    if (! router_any) {
        c.set_prop("router"sv, make_rc<qcm::Router>());
        router_any = c.prop("router"sv);
    }
    auto router = std::any_cast<rc<qcm::Router>>(router_any);
    return router;
}

static auto logout(ClientBase& cbase) -> asio::awaitable<void> {
    auto             c = *get_client(cbase);
    ncm::api::Logout api;
    co_await c.perform(api);
    co_return;
}
static auto session_check(ClientBase& cbase, helper::QWatcher<qcm::model::Session> session)
    -> asio::awaitable<Result<bool>> {
    auto                  c = *get_client(cbase);
    ncm::api::UserAccount api;
    auto                  out = co_await c.perform(api);
    co_await asio::post(
        asio::bind_executor(qcm::Global::instance()->qexecutor(), asio::use_awaitable));

    auto ex = co_await asio::this_coro::executor;

    auto user = session->user();
    co_return out.transform([user, ex, &session, c](const auto& out) -> bool {
        if (out.profile) {
            session->set_provider(convert_from<QString>(ncm::provider));
            user->set_userId(convert_from<ItemId>(out.profile->userId));
            user->set_nickname(convert_from<QString>(out.profile->nickname));
            user->set_avatarUrl(convert_from<QString>(out.profile->avatarUrl));
            user->query();
            asio::co_spawn(ex, prepare_session(c, user->userId()), helper::asio_detached_log);
            return true;
        }
        return false;
    });
}
void save(ClientBase& cbase, const std::filesystem::path& path) {
    auto c = *get_client(cbase);
    c.save(path);
}
void load(ClientBase& cbase, const std::filesystem::path& path) {
    auto c = *get_client(cbase);
    c.load(path);
}

bool make_request(ClientBase& cbase, request::Request& req, const QUrl& url,
                  const qcm::Client::ReqInfo& info) {
    auto c = *get_client(cbase);
    std::visit(overloaded { [c, &url, &req](const qcm::Client::ReqInfoImg& info) {
                   request::UrlParams query;
                   if (info.size.isValid()) {
                       auto down_size = get_down_size(info.size);
                       query.set_param("param",
                                       fmt::format("{}y{}", down_size.width(), down_size.height()));
                   }
                   req = c.make_req<ncm::api::CryptoType::NONE>(
                       convert_from<std::string>(url.toString()), query);
               } },
               info);
    return true;
}

auto collect(ClientBase& cbase, qcm::model::ItemId id, bool act) -> asio::awaitable<Result<bool>> {
    auto                c = *get_client(cbase);
    ncm::api::RadioLike api;
    api.input.like    = act;
    api.input.trackId = convert_from<model::SongId>(id);
    auto ok           = co_await c.perform(api);
    co_return ok.transform([](const ncm::api_model::RadioLike& out) {
        return out.code == 200;
    });
}

} // namespace ncm::impl

namespace ncm::qml
{

auto create_client() -> qcm::Client {
    auto c = ncm::Client(
        qcm::Global::instance()->session(),
        qcm::Global::instance()->pool_executor(),
        ncm::api::device_id_from_uuid(qcm::Global::instance()->uuid().toString().toStdString()));

    auto api      = make_rc<qcm::Global::Client::Api>();
    auto instance = make_rc<ncm::impl::Client>(c);

    api->provider      = ncm::qml::provider;
    api->server_url    = ncm::impl::server_url;
    api->make_request  = ncm::impl::make_request;
    api->play_state    = ncm::impl::play_state;
    api->router        = ncm::impl::router;
    api->logout        = ncm::impl::logout;
    api->session_check = ncm::impl::session_check;
    api->collect       = ncm::impl::collect;
    api->save          = ncm::impl::save;
    api->load          = ncm::impl::load;

    return { .api = api, .instance = instance };
}

auto get_ncm_client(qcm::Client& c) -> ncm::Client& {
    return static_cast<ncm::impl::Client&>(*c.instance).ncm;
}

auto uniq(const QUrl& url, const QVariant& info) -> QString {
    auto    size = info.value<QSize>();
    QString size_query;
    if (size.isValid()) {
        size = get_down_size(size);
    }
    return QString("%1&param=%2y%3").arg(url.toString()).arg(size.width()).arg(size.height());
}

} // namespace ncm::qml